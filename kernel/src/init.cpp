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
#include <debug.h>
#include <event/event_manager.hpp>
#include <fs/initrdfs.hpp>
#include <event/kernel_events.hpp>
#include <util/uuid.h>

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

        kernel::InitRdFilesystem* initrdfs = new kernel::InitRdFilesystem((void*)mod_start);
        kernel::VFS::instance()->mount(initrdfs, "/");
    }

    // Prepare device filesystem (/dev)
    new kernel::DeviceFilesystem();
    kernel::VFS::instance()->register_filesystem("devfs",
        [](kernel::VNodePtr) -> kernel::ValueOrError<kernel::Filesystem*> {
            return kernel::DeviceFilesystem::instance();
        });

    // Initialize the module manager
    auto* modMgr = new kernel::ModuleManager();
    modMgr->reload_modules();

    // Run the modules in modules.init
    modMgr->run_init_modules();
}

void transfer_function() {
    // At this stage all of the init modules should be set up
    // and we should be ready to launch the init process.
    // But first we need to free the init code and data

    const char* execFile = "/sbin/init";

    auto& thisProc = kernel::Thread::current()->parent();

    auto* sstream = new SimpleStream();
    thisProc.add_stream(sstream);
    thisProc.add_stream(sstream);
    thisProc.add_stream(sstream);

    asm volatile("mov $10, %%rax; mov $0, %%rcx; mov $0, %%rdx; int $0x8F" :: "b"(execFile));

    ASSERT_NOT_REACHED("We really shouldn't be here");
    while(true) asm volatile("hlt");
}

TEXT_FREE_AFTER_INIT void stage2_init() {
    kprintf("[%T] (Kernel) Multitasking initialized! Now in stage 2\n");

    init_debug();

    init_syscalls();

    // Initialize the event system
    new kernel::EventManager();

    /// TODO: [22.01.2023] Move tests into a module
    kernel::tests::do_tests();

    // Initialize the virtual filesystem
    new kernel::VFS();

    init_modules();

    // Wait for all events to be processed
    kernel::EventManager::get().wait(EVENT_QUEUE_EMPTY);

    transfer_function();
}

/// I think all of this should go into the init process
/*TEXT_FREE_AFTER_INIT kernel::VNodePtr get_fs_device(const char* devLocation) {
    if(!strcmp(devLocation, "none"))
        return nullptr;

    if(strfind(devLocation, "UUID") == devLocation || strfind(devLocation, "ID") == devLocation) {
        // Starts with "UUID" or "ID"
        const char* idStr = strchr(devLocation, '=') + 1; // Skip the prefix

        auto deviceRes = kernel::VFS::instance()->get_file(nullptr,
            (std::String("/dev/block/by-id/") + idStr).c_str(),
            { .resolve_link = true, .follow_links = true });

        return deviceRes ? *deviceRes : nullptr;
    }

    if(devLocation[0] == '/') {
        // Starts with "/" so it must be a path
        auto deviceRes = kernel::VFS::instance()->get_file(nullptr,
            devLocation,
            { .resolve_link = true, .follow_links = true });

        return deviceRes ? *deviceRes : nullptr;
    }

    // Unknown prefix or something...
    return nullptr;
}

const char WHITESPACE_CHARS[] = " \t";
TEXT_FREE_AFTER_INIT void mount_fstab() {
    auto fstabRes = kernel::VFS::instance()->get_file(nullptr, "/etc/fstab", { .resolve_link = true, .follow_links = true });
    if(!fstabRes)
        panic("Filesystem table file not found!");

    auto fstab = *fstabRes;
    kernel::FileStream stream(fstab);
    stream.open(FILE_OPEN_MODE_READ);

    size_t fstabSize = fstab->f_size / sizeof(char);
    char* buffer = new char[fstabSize + 1];
    stream.read(buffer, fstab->f_size);
    buffer[fstabSize] = 0;

    char* lineStart = buffer;
    while(*lineStart != 0) {
        // Find the next line break
        char* lineEnd = strchr(lineStart, '\n');
        // When no line break assume end of file as one
        if(!lineEnd)
            lineEnd = buffer + fstabSize;
        *lineEnd = 0;
        {
            // Find comment and "remove" it
            char* commentStart = strchr(lineStart, '#');
            if(commentStart)
                *commentStart = 0;

            // Blank line
            if(!*lineStart) goto next;

            // Take out the device name first
            char* whitespace = strchrs(lineStart, WHITESPACE_CHARS);
            if(!whitespace) goto next; // No whitespace found, skip this line as it's invalid
            *whitespace = 0;
            char* fsDev = lineStart;

            char* nextProp = strnchrs(whitespace + 1, WHITESPACE_CHARS);
            if(!nextProp) goto next; // Unexpected EOL

            whitespace = strchrs(nextProp, WHITESPACE_CHARS);
            if(!whitespace) goto next;
            *whitespace = 0;
            char* location = nextProp;

            nextProp = strnchrs(whitespace + 1, WHITESPACE_CHARS);
            if(!nextProp) goto next;

            whitespace = strchrs(nextProp, WHITESPACE_CHARS);
            if(whitespace) *whitespace = 0; // Ignore everything beyond the last property
            char* fsType = nextProp;

            auto fsDevNode = get_fs_device(fsDev);
            kernel::VFS::instance()->mount(fsDevNode, fsType, location);
        }
    next:
        lineStart = lineEnd + 1;
    }
}*/
