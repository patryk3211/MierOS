#include <stivale.h>
#include <sections.h>
#include <dmesg.h>
#include <memory/liballoc.h>
#include <memory/physical.h>
#include <cpu.h>

#include <assert.h>

#include <tests/test.hpp>

#include <function.hpp>

FREE_AFTER_INIT u8_t temp_stack[8192];

__attribute__((section(".stivale2hdr"), used))
static stivale2_header stivalehdr {
    .entry_point = 0,

    .stack = (u64_t)temp_stack + sizeof(temp_stack),

    .flags = (4 << 1) | (3 << 1) | (2 << 1) | (1 << 1),
    
    .tags = 0
};

extern "C" void _start() {
    stivale2_struct* structure;
    asm volatile("mov %%rdi, %0" : "=a"(structure));

    // Init serial for debug messages.
    init_serial();
    dmesg("\033[1;37mHello Serial!\033[0m\n");

    stivale2_stag_memmap* mem_map = 0;

    // Get all required tags from structure
    for(stivale2_tag_base* tag = (stivale2_tag_base*)structure->tags; tag != 0; tag = (stivale2_tag_base*)tag->next) {
        switch(tag->identifier) {
            case 0x2187f79e8612de07: // Memory Map Tag
                mem_map = (stivale2_stag_memmap*)tag;
                break;
        }
    }

    init_heap();
    init_pmm(mem_map);

    init_cpu();

    kernel::tests::do_tests();

    while(true);
}

extern "C" void __cxa_pure_virtual() {
    ASSERT_F(false, "You shouldn't be here");
    asm("int3");
}
