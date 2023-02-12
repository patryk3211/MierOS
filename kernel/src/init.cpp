#include <arch/cpu.h>
#include <defines.h>
#include <dmesg.h>
#include <fs/devicefs.hpp>
#include <fs/vfs.hpp>
#include <fs/vnode.hpp>
#include <memory/liballoc.h>
#include <memory/physical.h>
#include <memory/virtual.hpp>
#include <modules/module_manager.hpp>
#include <stivale.h>
#include <tasking/process.hpp>
#include <tasking/scheduler.hpp>
#include <tasking/syscall.h>
#include <tests/test.hpp>
#include <trace.h>
#include <event/event_manager.hpp>
#include <fs/initrdfs.hpp>
#include <event/kernel_events.hpp>
#include <util/uuid.h>
#include <fs/systemfs.hpp>

#ifdef x86_64
#include <arch/x86_64/acpi.h>
#endif

FREE_AFTER_INIT u8_t temp_stack[8192];

__attribute__((section(".stivale2hdr"), used)) static stivale2_header stivalehdr {
    .entry_point = 0,

    .stack = (u64_t)temp_stack + sizeof(temp_stack),

    .flags = (4 << 1) | (3 << 1) | (2 << 1) | (1 << 1),

    .tags = 0
};

extern "C" void (*_global_constructor_start)();
extern "C" void (*_global_constructor_end)();

extern "C" void (*_init_array_start)();
extern "C" void (*_init_array_end)();

FREE_AFTER_INIT stivale2_module mod;
FREE_AFTER_INIT stivale2_module sym_map;

void stage2_init();
extern "C" TEXT_FREE_AFTER_INIT void _start() {
    stivale2_struct* structure;
    asm volatile("mov %%rdi, %0"
                 : "=a"(structure));

    // Init serial for debug messages.
    init_serial();
    dmesg("\033[1;37mHello Serial!\033[0m");

    stivale2_stag_memmap* mem_map = 0;

    physaddr_t phys_base = 0;
    virtaddr_t virt_base = 0;

    stivale2_stag_pmrs* pmrs = 0;

    u64_t rsdp = 0;

    // Get all required tags from structure
    for(stivale2_tag_base* tag = (stivale2_tag_base*)structure->tags; tag != 0; tag = (stivale2_tag_base*)tag->next) {
        switch(tag->identifier) {
            case 0x2187f79e8612de07: // Memory Map Tag
                mem_map = (stivale2_stag_memmap*)tag;
                break;
            case 0x060d78874a2a8af0: // Kernel Base Address Tag
                phys_base = ((stivale2_stag_kernel_base*)tag)->physical_base_address;
                virt_base = ((stivale2_stag_kernel_base*)tag)->virtual_base_address;
                break;
            case 0x5df266a64047b6bd: // Protected Memory Ranges Tag
                pmrs = (stivale2_stag_pmrs*)tag;
                break;
            case 0x9e1786930a375e78: // ACPI RSDP Tag
                rsdp = ((stivale2_stag_rsdp*)tag)->rsdp_addr;
                break;
            case 0x4b6fe466aade04ce: { // Modules
                stivale2_stag_modules* mod_tag = (stivale2_stag_modules*)tag;
                for(size_t i = 0; i < mod_tag->module_count; ++i) {
                    if(!strcmp(mod_tag->modules[i].name, "module")) {
                        mod = mod_tag->modules[i];
                    } else if(!strcmp(mod_tag->modules[i].name, "map")) {
                        sym_map = mod_tag->modules[i];
                    }
                }
                break;
            }
        }
    }

    init_heap();

    // Call global constructors
    for(void (**contructor)() = &_global_constructor_start; contructor < &_global_constructor_end; ++contructor) {
        (*contructor)();
    }
    for(void (**constructor)() = &_init_array_start; constructor < &_init_array_end; ++constructor) {
        if(*constructor == 0 || *constructor == (void (*)())-1)
            continue;
        (*constructor)();
    }

    init_pmm(mem_map);

    early_init_cpu();

    // We have to get all the information from bootloader before pager initialization because only the kernel will be mapped after this point.
    kernel::Pager::init(phys_base, virt_base, pmrs);

#ifdef x86_64
    init_acpi(rsdp & 0x7FFFFFFFFFFF);
#endif

    init_cpu();

    /// pmm_release_bootloader_resources();

    kernel::Process* kern_proc = kernel::Process::construct_kernel_process((virtaddr_t)&stage2_init);
    kernel::Scheduler::schedule_process(*kern_proc);

    while(true) asm volatile("int $0xFE");
}

extern "C" void __cxa_pure_virtual() {
    ASSERT_NOT_REACHED("You shouldn't be here");
    asm("int3");
}

class SimpleStream : public kernel::Stream {
public:
    SimpleStream() : kernel::Stream(0xFF) {

    }

    virtual kernel::ValueOrError<size_t> read(void*, size_t) {
        return 0;
    }

    virtual kernel::ValueOrError<size_t> write(const void* buffer, size_t length) {
        char cbuf[length + 1];
        memcpy(cbuf, buffer, length);
        cbuf[length] = 0;

        char* ptr = cbuf;
        for(size_t i = 0; i < length; ++i) {
            if(cbuf[i] == '\n') {
                cbuf[i] = 0;
                if(i != 0) dmesg("%s", ptr);
                ptr = cbuf + i + 1;
            }
        }

        if(*ptr != 0)
            dmesg("%s", ptr);
        return length;
    }
};

TEXT_FREE_AFTER_INIT void init_filesystems() {
    // Create the VFS
    auto* vfs = new kernel::VFS();

    { // Find initrd loaded by bootloader
        physaddr_t mod_phys_start = mod.start & 0x7FFFFFFFFFFF;
        physaddr_t mod_phys_end = mod.end & 0x7FFFFFFFFFFF;
        size_t length = mod_phys_end - mod_phys_start;
        size_t page_size = (length >> 12) + ((length & 0xFFF) == 0 ? 0 : 1);

        physaddr_t map_phys_start = sym_map.start & 0x7FFFFFFFFFFF;
        physaddr_t map_phys_end = sym_map.end & 0x7FFFFFFFFFFF;
        size_t length2 = map_phys_end - map_phys_start;
        size_t page_size2 = (length2 >> 12) + ((length2 & 0xFFF) == 0 ? 0 : 1);

        kernel::Pager::kernel().lock();
        virtaddr_t mod_start = kernel::Pager::kernel().kmap(mod_phys_start, page_size, { 1, 0, 0, 0, 1, 0 });
        virtaddr_t map_start = kernel::Pager::kernel().kmap(map_phys_start, page_size2, { 1, 0, 0, 0, 1, 0 });
        kernel::Pager::kernel().unlock();

        set_line_map((void*)map_start);

        auto* initrdfs = new kernel::InitRdFilesystem((void*)mod_start);
        vfs->mount(initrdfs, "/");
    }

    // Prepare device filesystem (/dev)
    new kernel::DeviceFilesystem();
    vfs->register_filesystem("devfs",
        [](kernel::VNodePtr) -> kernel::ValueOrError<kernel::Filesystem*> {
            return kernel::DeviceFilesystem::instance();
        });

    // Prepare system filesystem (/sys)
    new kernel::SystemFilesystem();
    vfs->register_filesystem("sysfs",
        [](kernel::VNodePtr) -> kernel::ValueOrError<kernel::Filesystem*> {
            return kernel::SystemFilesystem::instance();
        });
}

void transfer_function() {
    // At this stage all of the init modules should be set up
    // and we should be ready to launch the init process.
    // But first we need to free the init code and data
    pmm_release_init_resources();

    const char* execFile = "/sbin/init";
    asm volatile("mov $10, %%rax; mov $0, %%rcx; mov $0, %%rdx; int $0x8F" :: "b"(execFile));

    panic("Failed to execute /sbin/init");
}

TEXT_FREE_AFTER_INIT void stage2_init() {
    dmesg("(Kernel) Multitasking initialized! Now in stage 2");

    init_syscalls();

    // Initialize the event system
    new kernel::EventManager();

    /// TODO: [22.01.2023] Move tests into a module
    kernel::tests::do_tests();

    // Initialize the filesystems
    init_filesystems();

    // Initialize the module manager
    new kernel::ModuleManager();

    auto& thisProc = kernel::Thread::current()->parent();

    auto* sstream = new SimpleStream();
    thisProc.add_stream(sstream);
    thisProc.add_stream(sstream);
    thisProc.add_stream(sstream);

    // We need the transfer function because we
    // will be freeing this one
    transfer_function();
}

