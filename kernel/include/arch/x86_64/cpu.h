#ifndef _MIEROS_KERNEL_ARCH_X86_64_CPU_H
#define _MIEROS_KERNEL_ARCH_X86_64_CPU_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern void wrmsr(u32_t msr, u64_t value);
extern u64_t rdmsr(u32_t msr);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ARCH_X86_64_CPU_H
