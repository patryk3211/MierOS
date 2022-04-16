#ifndef _MIEROS_KERNEL_ARCH_X86_64_CPU_H
#define _MIEROS_KERNEL_ARCH_X86_64_CPU_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern void wrmsr(u32_t msr, u64_t value);
extern u64_t rdmsr(u32_t msr);

extern void set_kernel_stack(int core, u64_t rsp);

extern void write_lapic(u32_t offset, u32_t value);
extern u32_t read_lapic(u32_t offset);

extern void send_ipi(u8_t vector, u8_t mode, u8_t destination);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ARCH_X86_64_CPU_H
