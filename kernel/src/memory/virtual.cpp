#include <memory/virtual.hpp>
#include <memory/physical.h>
#include <stdlib.h>
#include <sections.h>

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

// 0xFFFFFFFFFFFFF000 - Control the page table for page structure mappings.
// 0xFFFFFFFFFFE00000 - 0xFFFFFFFFFFFFE000 - Pages for mapping page structures.
#define CONTROL_PAGE ((PageStructuresEntry*)0xFFFFFFFFFFFFF000)
#define WORKPAGE(i) ((PageStructuresEntry*)(0xFFFFFFFFFFE00000 | ((i) << 12)))

#define REFRESH_TLB asm volatile("mov %cr3, %rax; mov %rax, %cr3");

physaddr_t Pager::kernel_pd[2] = { 0, 0 };

Pager::Pager() {
    this->pml4 = palloc(1);

    // Allocate a PML4
    int index = aquireWorkpageIndex();
    CONTROL_PAGE[index].raw = pml4 | 0x03;
    REFRESH_TLB;
    memset(WORKPAGE(index), 0, 4096);

    // Allocate a PDPT
    int index2 = aquireWorkpageIndex();
    physaddr_t pdpt = palloc(1);
    CONTROL_PAGE[index2].raw = pdpt | 0x03;
    REFRESH_TLB;
    memset(WORKPAGE(index2), 0, 4096);

    // Map the kernel pages
    WORKPAGE(index)[511].raw = pdpt | 0x03;
    WORKPAGE(index2)[510].raw = kernel_pd[0] | 0x03;
    WORKPAGE(index2)[511].raw = kernel_pd[1] | 0x03;

    releaseWorkpageIndex(index);
    releaseWorkpageIndex(index2);

    for(int i = 0; i < 6; ++i) work_pages[i] = -1;
}

Pager::~Pager() {
    // TODO: [21.01.2022] Free all page structures
}

extern "C" void* _kernel_end;
TEXT_FREE_AFTER_INIT void Pager::init(physaddr_t kernel_base_p, virtaddr_t kernel_base_v, stivale2_stag_pmrs* pmrs) {
    // We can access the addresses directly here, since the bootloader provides us with identity mappings.
    kernel_pd[0] = palloc(1);
    kernel_pd[1] = palloc(1);

    memset((void*)kernel_pd[0], 0, 4096);
    memset((void*)kernel_pd[1], 0, 4096);

    for(u64_t i = 0; i < pmrs->entry_count; ++i) {
        for(u64_t offset = 0; offset < pmrs->entries[i].length; offset += 0x1000) {
            u64_t addr = pmrs->entries[i].base+offset;
            u64_t perms = pmrs->entries[i].permissions;
            PageStructuresEntry& pde = ((PageStructuresEntry*)kernel_pd[0])[(addr >> 21) & 0x1FF];
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

    physaddr_t control_page = palloc(1);
    memset((void*)control_page, 0, 4096);
    ((PageStructuresEntry*)kernel_pd[1])[511].raw = control_page | 0x03;
    ((PageStructuresEntry*)control_page)[511].raw = control_page | 0x8000000000000103;
}

int Pager::aquireWorkpageIndex() {
    int index = 0;
    // This might need to be locked.
    PageStructuresEntry* control_page = (PageStructuresEntry*)0xFFFFFFFFFFFFF000;
    while(true) {
        if(!control_page[index].structured.present) {
            control_page[index].structured.present = 1;
            return index;
        }
        index = (index + 1) % 511;
    }
}

void Pager::releaseWorkpageIndex(int index) {
    PageStructuresEntry* control_page = (PageStructuresEntry*)0xFFFFFFFFFFFFF000;
    control_page[index].structured.present = 0;
}

void Pager::mapStructures(int new_pml4e, int new_pdpte, int new_pde) {
    // Map PML4 if not already mapped
    int pml4_work = work_pages[0];
    if(pml4_work == -1) {
        pml4_work = getWorkpage(0);
        CONTROL_PAGE[pml4_work].raw = pml4_work | 0x8000000000000103;
        REFRESH_TLB;
    }

    bool remap = false;
    int pdpt_work;
    // Remap the PDPT if required
    if(current_pml4e != new_pml4e) {
        remap = true;
        physaddr_t pdpt_addr = WORKPAGE(pml4_work)[new_pml4e].structured.address << 12;
        pdpt_work = getWorkpage(1);
        CONTROL_PAGE[pdpt_work].raw = pdpt_addr | 0x8000000000000103;
        REFRESH_TLB;
    }

    int pd_work;
    // Remap the PD if required
    if(current_pdpte != new_pdpte || remap) {
        remap = true;
        physaddr_t pd_addr = WORKPAGE(pdpt_work)[new_pdpte].structured.address << 12;
        pd_work = getWorkpage(2);
        CONTROL_PAGE[pd_work].raw = pd_addr | 0x8000000000000103;
        REFRESH_TLB;
    }

    int pt_work;
    // Remap the PT if required
    if(current_pde != new_pde || remap) {
        physaddr_t pt_addr = WORKPAGE(pd_work)[new_pde].structured.address << 12;
        pt_work = getWorkpage(3);
        CONTROL_PAGE[pt_work].raw = pt_addr | 0x8000000000000103;
        REFRESH_TLB;
    }
}

int Pager::getWorkpage(int index) {
    if(work_pages[index] == -1) work_pages[index] = aquireWorkpageIndex();
    return work_pages[index];
}

void Pager::lock() {
    locker.lock();
    current_pml4e = -1;
    current_pdpte = -1;
    current_pde = -1;
}

void Pager::unlock() {
    for(int i = 0; i < 6; ++i) {
        if(work_pages[i] != -1) {
            releaseWorkpageIndex(work_pages[i]);
            work_pages[i] = -1;
        }
    }
    locker.unlock();
}

void Pager::enable() {
    asm volatile("mov %0, %%cr3" : : "a"(pml4));
}
