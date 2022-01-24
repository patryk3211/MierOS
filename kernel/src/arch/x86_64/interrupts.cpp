#include <arch/interrupts.h>
#include <arch/x86_64/int_handlers.h>
#include <dmesg.h>
#include <stdlib.h>

struct interrupt_descriptor {
    u16_t offset_015;
    u16_t segment;
    u8_t reserved1;
    u8_t conf;
    u16_t offset_1631;
    u32_t offset_3263;
    u32_t reserved2;
}__attribute__((packed));

interrupt_descriptor idt[256];

void create_descriptor(interrupt_descriptor& entry, void (*handler)(), u8_t type, u8_t dpl) {
    entry.offset_015 = (u64_t)handler;
    entry.offset_1631 = (u64_t)handler >> 16;
    entry.offset_3263 = (u64_t)handler >> 32;

    entry.segment = 0x08;
    entry.reserved1 = 0;
    entry.reserved2 = 0;

    entry.conf = (type & 0xF) | ((dpl & 0x03) << 5) | (1 << 7);
}

struct idt_ptr {
    u16_t length;
    u64_t ptr;
}__attribute__((packed));

idt_ptr idtr;

struct handler_entry {
    void (*handler)();
    handler_entry* next;
};

handler_entry* handlers[256];
CPUState* (*_tsh)(CPUState* current_state);
u32_t (*_sh)(u32_t, u32_t, u32_t, u32_t, u32_t, u32_t);

extern "C" void init_interrupts() {
    memset(handlers, 0, sizeof(handlers));
    _tsh = 0;

    for(int i = 0; i < 256; ++i) create_descriptor(idt[i], &int_ignore, 0xE, 0);

    create_descriptor(idt[0x00], &interrupt0x00, 0xE, 0);
    create_descriptor(idt[0x01], &interrupt0x01, 0xE, 0);
    create_descriptor(idt[0x02], &interrupt0x02, 0xE, 0);
    create_descriptor(idt[0x03], &interrupt0x03, 0xE, 0);
    create_descriptor(idt[0x04], &interrupt0x04, 0xE, 0);
    create_descriptor(idt[0x05], &interrupt0x05, 0xE, 0);
    create_descriptor(idt[0x06], &interrupt0x06, 0xE, 0);
    create_descriptor(idt[0x07], &interrupt0x07, 0xE, 0);
    create_descriptor(idt[0x08], &interrupt0x08, 0xE, 0);
    create_descriptor(idt[0x09], &interrupt0x09, 0xE, 0);
    create_descriptor(idt[0x0A], &interrupt0x0A, 0xE, 0);
    create_descriptor(idt[0x0B], &interrupt0x0B, 0xE, 0);
    create_descriptor(idt[0x0C], &interrupt0x0C, 0xE, 0);
    create_descriptor(idt[0x0D], &interrupt0x0D, 0xE, 0);
    create_descriptor(idt[0x0E], &interrupt0x0E, 0xE, 0);
    create_descriptor(idt[0x0F], &interrupt0x0F, 0xE, 0);
    create_descriptor(idt[0x10], &interrupt0x10, 0xE, 0);
    create_descriptor(idt[0x11], &interrupt0x11, 0xE, 0);
    create_descriptor(idt[0x12], &interrupt0x12, 0xE, 0);
    create_descriptor(idt[0x13], &interrupt0x13, 0xE, 0);
    create_descriptor(idt[0x14], &interrupt0x14, 0xE, 0);
    create_descriptor(idt[0x15], &interrupt0x15, 0xE, 0);
    create_descriptor(idt[0x16], &interrupt0x16, 0xE, 0);
    create_descriptor(idt[0x17], &interrupt0x17, 0xE, 0);
    create_descriptor(idt[0x18], &interrupt0x18, 0xE, 0);
    create_descriptor(idt[0x19], &interrupt0x19, 0xE, 0);
    create_descriptor(idt[0x1A], &interrupt0x1A, 0xE, 0);
    create_descriptor(idt[0x1B], &interrupt0x1B, 0xE, 0);
    create_descriptor(idt[0x1C], &interrupt0x1C, 0xE, 0);
    create_descriptor(idt[0x1D], &interrupt0x1D, 0xE, 0);
    create_descriptor(idt[0x1E], &interrupt0x1E, 0xE, 0);
    create_descriptor(idt[0x1F], &interrupt0x1F, 0xE, 0);

    idtr.length = sizeof(idt);
    idtr.ptr = (u64_t)idt;

    asm volatile("lidt %0; sti" : : "m"(idtr));
}

extern "C" u64_t interrupt_handle(u64_t rsp) {
    CPUState* state = (CPUState*)rsp;
    if(state->int_num == 0xFE) {
        return (u64_t)_tsh(state);
    } else if(state->int_num == 0x8F) {
        state->rax = _sh(state->rbx,
                         state->rcx,
                         state->rdx,
                         state->rsi,
                         state->rdi,
                         state->rbp);
        return rsp;
    } else {
        for(handler_entry* handler = handlers[state->int_num]; handler != 0; handler = handler->next)
            handler->handler();
    }
    return rsp;
}

extern "C" void register_handler(u8_t vector, void (*handler)()) {
    handler_entry* entry = new handler_entry();
    entry->next = 0;
    entry->handler = handler;
    
    if(handlers[vector] == 0) handlers[vector] = entry;
    else {
        handler_entry* last = handlers[vector];
        while(last->next != 0) last = last->next;
        last->next = entry;
    }
}

extern "C" void register_task_switch_handler(CPUState* (*handler)(CPUState* current_state)) {
    _tsh = handler;
}

extern "C" void register_syscall_handler(u32_t (*handler)(u32_t, u32_t, u32_t, u32_t, u32_t, u32_t)) {
    _sh = handler;
}