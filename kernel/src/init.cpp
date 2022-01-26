#include <stivale.h>
#include <sections.h>
#include <dmesg.h>
#include <arch/cpu.h>
#include <memory/liballoc.h>
#include <memory/physical.h>
#include <memory/virtual.hpp>
#include <tests/test.hpp>
#include <tasking/process.hpp>
#include <tasking/scheduler.hpp>

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

void stage2_init() {
    dmesg("[Kernel] Multitasking initialized! Now in stage 2\n");
    kernel::tests::do_tests();

    while(true);
}
