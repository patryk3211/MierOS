#include <arch/interrupts.h>

struct interrupt_descriptor {

}__attribute__((packed));

extern "C" void init_interrupts() {

}

extern "C" void register_handler(u8_t vector, void (*handler)()) {

}

extern "C" void register_task_switch_handler(CPUState* (*handler)(CPUState* current_state)) {

}

extern "C" void register_syscall_handler(u32_t (*handler)(u32_t, u32_t, u32_t, u32_t, u32_t, u32_t)) {

}