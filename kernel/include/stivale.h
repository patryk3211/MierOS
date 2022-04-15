#ifndef _MIEROS_KERNEL_STIVALE_H
#define _MIEROS_KERNEL_STIVALE_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct stivale2_header {
    u64_t entry_point;
    u64_t stack;
    u64_t flags;
    u64_t tags;
};

struct stivale2_struct {
    char bootloader_brand[64];
    char bootloader_version[64];

    u64_t tags;
};

struct stivale2_tag_base {
    u64_t identifier;
    u64_t next;
};

struct stivale2_stag_kernel_base {
    struct stivale2_tag_base base;
    u64_t physical_base_address;
    u64_t virtual_base_address;
};

#define STIVALE2_PMR_EXECUTABLE ((u64_t)1 << 0)
#define STIVALE2_PMR_WRITABLE ((u64_t)1 << 1)
#define STIVALE2_PMR_READABLE ((u64_t)1 << 2)

struct stivale2_pmr {
    u64_t base;
    u64_t length;
    u64_t permissions;
};

struct stivale2_stag_pmrs {
    struct stivale2_tag_base base;
    u64_t entry_count;
    struct stivale2_pmr entries[];
};

#define STIVALE2_MEMMAP_TYPE_USABLE 1
#define STIVALE2_MEMMAP_TYPE_RESERVED 2
#define STIVALE2_MEMMAP_TYPE_ACPI_RECLAIMABLE 3
#define STIVALE2_MEMMAP_TYPE_ACPI_NVS 4
#define STIVALE2_MEMMAP_TYPE_BAD_MEM 5
#define STIVALE2_MEMMAP_TYPE_BOOTLOADER_RECLAIMABLE 0x1000
#define STIVALE2_MEMMAP_TYPE_KERNEL_AND_MODULES 0x1001
#define STIVALE2_MEMMAP_TYPE_FRAMEBUFFER 0x1002

struct stivale2_memmap_entry {
    u64_t base;
    u64_t length;
    u32_t type;
    u32_t unused;
};

struct stivale2_stag_memmap {
    struct stivale2_tag_base base;
    u64_t entry_count;
    struct stivale2_memmap_entry entries[];
};

struct stivale2_stag_rsdp {
    struct stivale2_tag_base base;
    u64_t rsdp_addr;
};

struct stivale2_module {
    u64_t start;
    u64_t end;
    char name[128];
};

struct stivale2_stag_modules {
    struct stivale2_tag_base base;
    u64_t module_count;
    struct stivale2_module modules[];
};

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_STIVALE_H
