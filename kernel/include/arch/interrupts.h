#ifndef _MIEROS_KERNEL_ARCH_INTERRUPTS_H
#define _MIEROS_KERNEL_ARCH_INTERRUPTS_H

#include <types.h>
#include <tasking/cpu_state.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern void init_interrupts();

extern void register_handler(u8_t vector, void (*handler)());
extern void register_task_switch_handler(CPUState* (*handler)(CPUState* current_state));
extern void register_syscall_handler(u32_t (*handler)(u32_t, u32_t, u32_t, u32_t, u32_t, u32_t));

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ARCH_INTERRUPTS_H


#include <types.h>
#include <tasking/cpu_state.h>
