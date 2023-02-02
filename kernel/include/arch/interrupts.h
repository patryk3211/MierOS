#ifndef _MIEROS_KERNEL_ARCH_INTERRUPTS_H
#define _MIEROS_KERNEL_ARCH_INTERRUPTS_H

#include <tasking/cpu_state.h>
#include <tasking/syscall.h>
#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern void init_interrupts();

extern void register_handler(u8_t vector, void (*handler)());
extern void register_task_switch_handler(CPUState* (*handler)(CPUState* current_state));
extern void register_syscall_handler(syscall_arg_t (*handler)(syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t));
extern void unregister_handler(u8_t vector, void (*handler)());

extern void force_task_switch();
extern void send_task_switch_irq(int core);

extern void enter_critical();
extern void leave_critical();

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ARCH_INTERRUPTS_H

#include <tasking/cpu_state.h>
#include <types.h>
