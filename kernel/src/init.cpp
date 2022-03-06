#include <stivale.h>
#include <defines.h>
#include <arch/cpu.h>
#include <tests/test.hpp>
#include <memory/liballoc.h>
#include <memory/physical.h>
#include <memory/virtual.hpp>
#include <tasking/process.hpp>
#include <tasking/scheduler.hpp>
#include <modules/module_manager.hpp>
#include <initrd.h>
#include <dmesg.h>
#include <trace.h>
#include <fs/devicefs.hpp>
#include <fs/vnode.hpp>

#ifdef x86_64
    #include <arch/x86_64/acpi.h>
#endif

FREE_AFTER_INIT u8_t temp_stack[8192];

__attribute__((section(".stivale2hdr"), used))
static stivale2_header stivalehdr {
    .entry_point = 0,

    .stack = (u64_t)temp_stack + sizeof(temp_stack),

    .flags = (4 << 1) | (3 << 1) | (2 << 1) | (1 << 1),
    
    .tags = 0
};

extern "C" void (*_global_constructor_start)();
extern "C" void (*_global_constructor_end)();

FREE_AFTER_INIT stivale2_module mod;
FREE_AFTER_INIT stivale2_module sym_map;

void stage2_init();
extern "C" TEXT_FREE_AFTER_INIT void _start() {
    stivale2_struct* structure;
    asm volatile("mov %%rdi, %0" : "=a"(structure));

    // Init serial for debug messages.
    init_serial();
    dmesg("\033[1;37mHello Serial!\033[0m\n");

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
            case 0x4b6fe466aade04ce: {// Modules
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

    init_pmm(mem_map);

    early_init_cpu();

    // We have to get all the information from bootloader before pager initialization because only the kernel will be mapped after this point.
    kernel::Pager::init(phys_base, virt_base, pmrs);

#ifdef x86_64
    init_acpi(rsdp & 0x7FFFFFFFFFFF);
#endif

    init_cpu();

    pmm_release_bootloader_resources();

    kernel::Process* kern_proc = kernel::Process::construct_kernel_process((virtaddr_t)&stage2_init);
    kernel::Scheduler::schedule_process(*kern_proc);
    asm volatile("int $0xFE");
    
    ASSERT_NOT_REACHED("You should be in stage2 right now");
    while(true) asm volatile("hlt");
}

extern "C" void __cxa_pure_virtual() {
    ASSERT_NOT_REACHED("You shouldn't be here");
    asm("int3");
}

TEXT_FREE_AFTER_INIT void stage2_init() {
    dmesg("[Kernel] Multitasking initialized! Now in stage 2\n");
    kernel::tests::do_tests();

    physaddr_t mod_phys_start = mod.start & 0x7FFFFFFFFFFF;
    physaddr_t mod_phys_end = mod.end & 0x7FFFFFFFFFFF;
    size_t length = mod_phys_end - mod_phys_start;
    size_t page_size = (length >> 12) + ((length & 0xFFF) == 0 ? 0 : 1);

    physaddr_t map_phys_start = sym_map.start & 0x7FFFFFFFFFFF;
    physaddr_t map_phys_end = sym_map.end & 0x7FFFFFFFFFFF;
    size_t length2 = map_phys_end - map_phys_start;
    size_t page_size2 = (length2 >> 12) + ((length2 & 0xFFF) == 0 ? 0 : 1);

    kernel::Pager::kernel().lock();
    virtaddr_t mod_start = kernel::Pager::kernel().kmap(mod_phys_start, page_size, { .present = 1, .writable = 0, .user_accesible = 0, .executable = 0, .global = 1, .cache_disable = 0 });
    virtaddr_t map_start = kernel::Pager::kernel().kmap(map_phys_start, page_size2, { .present = 1, .writable = 0, .user_accesible = 0, .executable = 0, .global = 1, .cache_disable = 0 });
    kernel::Pager::kernel().unlock();

    set_line_map((void*)map_start);
    set_initrd((void*)mod_start);

    void** files = get_files("*.mod");
    for(size_t i = 0; files[i] != 0; ++i) {
        kernel::add_preloaded_module(files[i]);
    }

    new kernel::DeviceFilesystem();

    kernel::init_modules("INIT", 0);

    auto devices = kernel::DeviceFilesystem::instance()->get_files(0, "", { .resolve_link = 0, .follow_links = 1 });
    for(auto& dev : *devices) {
        dmesg(dev->name().c_str());
        dmesg(" ");
    }
    dmesg("\nBy ID:\n");
    devices = kernel::DeviceFilesystem::instance()->get_files(0, "block/by-id", { .resolve_link = 0, .follow_links = 1 });
    for(auto& dev : *devices) {
        dmesg(dev->name().c_str());
        dmesg(" ");
    }
    dmesg("\n");

    while(true);
}
