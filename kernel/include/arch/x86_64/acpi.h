#ifndef _MIEROS_KERNEL_ARCH_X86_64_ACPI_H
#define _MIEROS_KERNEL_ARCH_X86_64_ACPI_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern void init_acpi(physaddr_t rsdp);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ARCH_X86_64_ACPI_H
