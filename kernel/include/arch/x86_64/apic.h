#ifndef _MIEROS_KERNEL_ARCH_X86_64_APIC_H
#define _MIEROS_KERNEL_ARCH_X86_64_APIC_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern void add_ioapic(u8_t id, u32_t address, u32_t globalSystemInterrupBase);
extern void add_ioapic_intentry(u8_t interruptVector, u32_t globalSystemInterrupt, u8_t triggerMode, u8_t polarity, u8_t mask);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ARCH_X86_64_APIC_H
