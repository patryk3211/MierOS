#ifndef _MIEROS_KERNEL_ARCH_X86_64_ACPI_H
#define _MIEROS_KERNEL_ARCH_X86_64_ACPI_H

#include <defines.h>
#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct ACPI_SDTHeader {
    char sign[4];
    u32_t length;
    u8_t revision;
    u8_t checksum;

    char oemId[6];
    char oemTableId[8];
    u32_t oemRevision;

    u32_t creatorId;
    u32_t creatorRevision;
} PACKED;

struct ACPI_AddressStructure {
    u8_t addressSpace;
    u8_t bitWidth;
    u8_t bitOffset;
    u8_t accessSize;
    u64_t address;
} PACKED;

extern void init_acpi(physaddr_t rsdp);
extern physaddr_t get_table(const char* sign);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ARCH_X86_64_ACPI_H
