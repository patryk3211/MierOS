#include <memory/virtual.hpp>
#include <memory/physical.h>
#include <stdlib.h>

using namespace kernel;

union PageStructuresEntry {
    struct {
        u64_t present:1;
        u64_t write:1;
        u64_t user:1;
        u64_t write_through:1;
        u64_t cache_disabled:1;
        u64_t accesed:1;
        u64_t dirty:1;
        u64_t pat_ps:1;
        u64_t global:1;
        u64_t available:3;
        u64_t address:40;
        u64_t available2:7;
        u64_t protection_key:4;
        u64_t execute_disable:1;
    } structured;
    u64_t raw;
};

Pager::Pager() {

}

Pager::~Pager() {

}

extern "C" void* _kernel_end;
void Pager::init(physaddr_t kernel_base_p, virtaddr_t kernel_base_v, stivale2_stag_pmrs* pmrs) {
    // We can access the addresses directly here, since the bootloader provides us with identity mappings.
    physaddr_t init_pml4 = palloc(1);
    physaddr_t init_pdpt = palloc(1);
    physaddr_t init_pd = palloc(1);

    memset((void*)init_pml4, 0, 4096);
    memset((void*)init_pdpt, 0, 4096);
    memset((void*)init_pd, 0, 4096);

    ((PageStructuresEntry*)init_pml4)[511].raw = init_pdpt | 0x03;
    ((PageStructuresEntry*)init_pdpt)[510].raw = init_pd | 0x03;

    /*virtaddr_t addr = kernel_base_v;
    while(addr < (virtaddr_t)&_kernel_end) {
        physaddr_t pt;
        if(((PageStructuresEntry*)init_pd)[(addr >> 21) & 0x1FF].raw == 0) {
            pt = palloc(1);
            memset((void*)pt, 0, 4096);
            ((PageStructuresEntry*)init_pd)[(addr >> 21) & 0x1FF].raw = pt | 0x03;
        }
        ((PageStructuresEntry*)pt)[(addr >> 12) & 0x1FF].raw = (addr - kernel_base_v + kernel_base_p) | 0x103;
        addr += 0x1000;
    }*/
    for(u64_t i = 0; i < pmrs->entry_count; ++i) {
        for(u64_t offset = 0; offset < pmrs->entries[i].length; offset += 0x1000) {
            u64_t addr = pmrs->entries[i].base+offset;
            u64_t perms = pmrs->entries[i].permissions;
            PageStructuresEntry& pde = ((PageStructuresEntry*)init_pd)[(addr >> 21) & 0x1FF];
            physaddr_t pt = 0;
            if(!pde.structured.present) {
                pt = palloc(1);
                pde.raw = pt;
                pde.structured.present = 1;
                pde.structured.write = 1;
                memset((void*)pt, 0, 4096);
            } else pt = pde.raw & ~0xFFF;
            PageStructuresEntry& pte = ((PageStructuresEntry*)pt)[(addr >> 12) & 0x1FF];
            pte.raw = addr - kernel_base_v + kernel_base_p;
            pte.structured.present = 1;
            pte.structured.global = 1;
            if(perms & STIVALE2_PMR_WRITABLE) pte.structured.write = 1;
            if(!(perms & STIVALE2_PMR_EXECUTABLE)) pte.structured.execute_disable = 1;
        }
    }

    asm volatile("movq %0, %%cr3" : : "a"(init_pml4));
}
