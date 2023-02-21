#ifndef _MIEROS_KERNEL_MEMORY_PHYSICAL_H
#define _MIEROS_KERNEL_MEMORY_PHYSICAL_H

#include <stivale.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern void init_pmm(struct stivale2_stag_memmap* memory_map);
extern void pmm_release_bootloader_resources();
extern void pmm_release_init_resources();

extern physaddr_t palloc(size_t page_count);
extern void pfree(physaddr_t addr, size_t page_count);
//extern void preserve(physaddr_t addr, size_t page_count);

extern size_t allocated_ppage_count();

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_MEMORY_PHYSICAL_H
