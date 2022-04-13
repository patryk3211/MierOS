#ifndef _MIEROS_KERNEL_ARCH_X86_64_HPET_H
#define _MIEROS_KERNEL_ARCH_X86_64_HPET_H

#include <types.h>
#include <defines.h>
#include <arch/x86_64/acpi.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct ACPI_HPET_Table {
    struct ACPI_SDTHeader header;
    u8_t hardwareRevision;
    u8_t comparatorCount:5;
    u8_t counterSize:1;
    u8_t reserved:1;
    u8_t legacyReplacement:1;
    u16_t pciVendor;
    struct ACPI_AddressStructure address;
    u8_t hpetNumber;
    u16_t minimumTick;
    u8_t pageProtection;
} PACKED;

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ARCH_X86_64_HPET_H
