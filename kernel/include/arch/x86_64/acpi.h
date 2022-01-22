#ifndef _MIEROS_KERNEL_ARCH_X86_64_ACPI_H
#define _MIEROS_KERNEL_ARCH_X86_64_ACPI_H

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
}__attribute__((packed));

struct ACPI_MADT {
    struct ACPI_SDTHeader header;

    u32_t localApicAddr;
    u32_t flags;

    u8_t records[0];
}__attribute__((packed));

extern void init_acpi(physaddr_t rsdp);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ARCH_X86_64_ACPI_H
