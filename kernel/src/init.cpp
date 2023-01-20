#include <arch/cpu.h>
#include <defines.h>
#include <dmesg.h>
#include <fs/devicefs.hpp>
#include <fs/vfs.hpp>
#include <fs/vnode.hpp>
#include <initrd.h>
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
#include <debug.h>
#include <event/event_manager.hpp>
#include <fs/initrdfs.hpp>

#include <fs/modulefs.hpp>
#include <fs/modulefs_functions.hpp>

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

    virtual size_t read(void*, size_t) {
        return 0;
    }

    virtual size_t write(const void* buffer, size_t length) {
        dmesgl((char*)buffer, length);
        return length;
    }
};

TEXT_FREE_AFTER_INIT void init_modules() {
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
        //set_initrd((void*)mod_start);

        kernel::InitRdFilesystem* initrdfs = new kernel::InitRdFilesystem((void*)mod_start);
        kernel::VFS::instance()->mount(initrdfs, "/");
    }

    // Initialize the module manager
    new kernel::ModuleManager();

    auto file = kernel::VFS::instance()->get_file(nullptr, "/etc/modules.init", { });
    if(file) {
        auto stream = kernel::FileStream(*file);
        stream.open(FILE_OPEN_MODE_READ);

        char buffer[stream.node()->f_size + 1];
        buffer[stream.node()->f_size] = 0;
        stream.read(buffer, stream.node()->f_size);

        kprintf("[%T] File contents '%s'\n", buffer);
    }

    while(true);
//    // Preload all .mod files as modules
//    void** files = get_files("*.mod");
//    for(size_t i = 0; files[i] != 0; ++i)
//        kernel::ModuleManager::get().preload_module(files[i]);
//
//    // Prepare device filesystem (/dev)
//    new kernel::DeviceFilesystem();
//
//    // Load modules specifies by modules.init file
//    char* initModules = (char*)get_file("modules.init");
}

TEXT_FREE_AFTER_INIT void stage2_init() {
    kprintf("[%T] (Kernel) Multitasking initialized! Now in stage 2\n");

    init_debug();

    init_syscalls();

    // Initialize the event system
    new kernel::EventManager();

    kernel::tests::do_tests();

    // Initialize the virtual filesystem
    new kernel::VFS();

    init_modules();

    {
        auto devices = kernel::DeviceFilesystem::instance()->get_files(nullptr, "", { .resolve_link = 0, .follow_links = 1 });
        for(auto& dev : *devices) {
            kprintf(dev->name().c_str());
            kprintf(" ");
        }
        kprintf("\nBy ID:\n");
        devices = kernel::DeviceFilesystem::instance()->get_files(nullptr, "block/by-id", { .resolve_link = 0, .follow_links = 1 });
        for(auto& dev : *devices) {
            kprintf(dev->name().c_str());
            kprintf(" ");
        }
        kprintf("\n");
    }

    /*u16_t fs_mod = kernel::init_modules("FS-ext2", 0);
    kernel::Thread::current()->f_current_module = kernel::get_module(fs_mod);
    auto* fs_mount = kernel::get_module_symbol<kernel::fs_function_table>(fs_mod, "fs_func_tab")->mount;
    u16_t minor = *fs_mount(*kernel::DeviceFilesystem::instance()->get_file(nullptr, "ahci0p1", {}));
    kernel::ModuleFilesystem* mfs = new kernel::ModuleFilesystem(fs_mod, minor);*/

    //vfs->mount(mfs, "/");

    /*auto result = vfs->get_files(nullptr, "", {});
    if(result) {
        auto nodes = *result;
        for(auto& node : nodes) {
            kprintf("%s ", node->name().c_str());
        }
        kprintf("\n");
    }
    kernel::Thread::current()->f_current_module = 0;*/

    TRACE("Test\n");

    const char* execFile = "/test_user";

    auto& thisProc = kernel::Thread::current()->parent();

    auto* sstream = new SimpleStream();
    thisProc.add_stream(sstream);
    thisProc.add_stream(sstream);
    thisProc.add_stream(sstream);

    asm volatile("mov $10, %%rax; mov $0, %%rcx; mov $0, %%rdx; int $0x8F" :: "b"(execFile));

    while(true) asm volatile("hlt");
}
