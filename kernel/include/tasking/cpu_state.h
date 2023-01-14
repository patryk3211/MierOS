#ifndef _MIEROS_KERNEL_TASKING_CPU_STATE_H
#define _MIEROS_KERNEL_TASKING_CPU_STATE_H

#include <defines.h>
#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct CPUState {
    u64_t next_switch_time;

    u64_t fs;

    u64_t cr3;

    u64_t rax;
    u64_t rbx;
    u64_t rcx;
    u64_t rdx;

    u64_t rsi;
    u64_t rdi;

    u64_t r8;
    u64_t r9;
    u64_t r10;
    u64_t r11;
    u64_t r12;
    u64_t r13;
    u64_t r14;
    u64_t r15;

    u64_t rbp;

    u64_t int_num;
    u64_t err_code;

    u64_t rip;
    u64_t cs;
    u64_t rflags;

    u64_t rsp;
    u64_t ss;
} PACKED;

#define CPUSTATE_IP(state) state->rip
#define CPUSTATE_RET(state) state->rax

extern void cpu_state_dump(CPUState* state);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_TASKING_CPU_STATE_H
