#ifndef _MIEROS_KERNEL_ARCH_X86_64_HPET_H
#define _MIEROS_KERNEL_ARCH_X86_64_HPET_H

#include <arch/x86_64/acpi.h>
#include <defines.h>
#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct ACPI_HPET_Table {
    struct ACPI_SDTHeader header;
    u8_t hardwareRevision;
    u8_t comparatorCount   : 5;
    u8_t counterSize       : 1;
    u8_t reserved          : 1;
    u8_t legacyReplacement : 1;
    u16_t pciVendor;
    struct ACPI_AddressStructure address;
    u8_t hpetNumber;
    u16_t minimumTick;
    u8_t pageProtection;
} PACKED;

struct HPET_Timer {
    u64_t config_and_capabilities;
    u64_t comparator_value;
    u64_t fsb_route;
    u64_t reserved;
} PACKED;

struct HPET {
    u64_t capabilities_and_id;
    u64_t pad1;
    u64_t configuration;
    u64_t pad2;
    u64_t interrupt_status;
    u64_t pad3[25];
    u64_t counter_value;
    u64_t pad4;
    struct HPET_Timer timers[32];
} PACKED;

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ARCH_X86_64_HPET_H
