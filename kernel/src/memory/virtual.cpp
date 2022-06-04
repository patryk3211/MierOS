#include <defines.h>
#include <locking/locker.hpp>
#include <memory/physical.h>
#include <memory/virtual.hpp>
#include <stdlib.h>

using namespace kernel;

union PageStructuresEntry {
    struct {
        u64_t present         : 1;
        u64_t write           : 1;
        u64_t user            : 1;
        u64_t write_through   : 1;
        u64_t cache_disabled  : 1;
        u64_t accesed         : 1;
        u64_t dirty           : 1;
        u64_t pat_ps          : 1;
        u64_t global          : 1;
        u64_t available       : 3;
        u64_t address         : 40;
        u64_t available2      : 7;
        u64_t protection_key  : 4;
        u64_t execute_disable : 1;
    } structured;
    u64_t raw;
};

// 0xFFFFFFFFFFFFF000 - Control the page table for page structure mappings.
// 0xFFFFFFFFFFE00000 - 0xFFFFFFFFFFFFE000 - Pages for mapping page structures.
#define CONTROL_PAGE ((PageStructuresEntry*)0xFFFFFFFFFFFFF000)
#define WORKPAGE(i) ((PageStructuresEntry*)(0xFFFFFFFFFFE00000 | ((i) << 12)))

#define BASE_MAPPING_ADDRESS 0x1000

#define REFRESH_TLB asm volatile("mov %cr3, %rax; mov %rax, %cr3");

NO_EXPORT physaddr_t Pager::kernel_pd[2] = { 0, 0 };
NO_EXPORT std::List<Pager*> Pager::pagers;
NO_EXPORT SpinLock Pager::kernel_locker;
NO_EXPORT virtaddr_t Pager::first_potential_kernel_page;
NO_EXPORT Pager* Pager::kernel_pager = 0;

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
    WORKPAGE(index)
    [511].raw
        = pdpt | 0x03;
    WORKPAGE(index2)
    [510].raw
        = kernel_pd[0] | 0x03;
    WORKPAGE(index2)
    [511].raw
        = kernel_pd[1] | 0x03;

    releaseWorkpageIndex(index);
    releaseWorkpageIndex(index2);

    for(int i = 0; i < MAX_WORK_PAGES; ++i) work_pages[i] = -1;

    first_potential_page = BASE_MAPPING_ADDRESS;

    pagers.push_back(this);
}

Pager::~Pager() {
    for(auto iter = pagers.begin(); iter != pagers.end(); ++iter) {
        if((*iter)->pml4 == pml4) {
            pagers.erase(iter);
            break;
        }
    }
    /// TODO: [21.01.2022] Free all page structures
}

extern "C" void* _kernel_end;
TEXT_FREE_AFTER_INIT void Pager::init(physaddr_t kernel_base_p, virtaddr_t kernel_base_v, stivale2_stag_pmrs* pmrs) {
    // We can access the addresses directly here, since the bootloader provides us with identity mappings.
    physaddr_t init_pml4 = palloc(1);
    physaddr_t init_pdpt = palloc(1);
    kernel_pd[0] = palloc(1);
    kernel_pd[1] = palloc(1);

    memset((void*)kernel_pd[0], 0, 4096);
    memset((void*)kernel_pd[1], 0, 4096);

    ((PageStructuresEntry*)init_pml4)[511].raw = init_pdpt | 0x03;
    ((PageStructuresEntry*)init_pdpt)[510].raw = kernel_pd[0] | 0x03;
    ((PageStructuresEntry*)init_pdpt)[511].raw = kernel_pd[1] | 0x03;

    for(u64_t i = 0; i < pmrs->entry_count; ++i) {
        for(u64_t offset = 0; offset < pmrs->entries[i].length; offset += 0x1000) {
            u64_t addr = pmrs->entries[i].base + offset;
            u64_t perms = pmrs->entries[i].permissions;
            PageStructuresEntry& pde = ((PageStructuresEntry*)kernel_pd[0])[(addr >> 21) & 0x1FF];
            physaddr_t pt = 0;
            if(!pde.structured.present) {
                pt = palloc(1);
                pde.raw = pt;
                pde.structured.present = 1;
                pde.structured.write = 1;
                memset((void*)pt, 0, 4096);
            } else
                pt = pde.raw & ~0xFFF;
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

    kernel_locker = SpinLock();

    asm volatile("mov %0, %%cr3"
                 :
                 : "a"(init_pml4));

    first_potential_kernel_page = KERNEL_START;

    // Create the first pager.
    Pager* p = new Pager();
    p->enable();
    pfree(init_pml4, 1);
    pfree(init_pdpt, 1);

    kernel_pager = p;
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
        CONTROL_PAGE[pml4_work].raw = pml4 | 0x8000000000000003;
        REFRESH_TLB;
    }

    bool remap = false;
    int pdpt_work;
    // Remap the PDPT if required
    if(current_pml4e != new_pml4e) {
        remap = true;
        pdpt_work = getWorkpage(1);
        if(WORKPAGE(pml4_work)[new_pml4e].structured.present) {
            physaddr_t pdpt_addr = WORKPAGE(pml4_work)[new_pml4e].structured.address << 12;
            CONTROL_PAGE[pdpt_work].raw = pdpt_addr | 0x8000000000000003;
            REFRESH_TLB;
        } else {
            physaddr_t pdpt_addr = palloc(1);
            WORKPAGE(pml4_work)
            [new_pml4e].raw
                = pdpt_addr | 0x07;
            CONTROL_PAGE[pdpt_work].raw = pdpt_addr | 0x8000000000000003;
            REFRESH_TLB;
            memset(WORKPAGE(pdpt_work), 0, 4096);
        }
    }

    int pd_work;
    // Remap the PD if required
    if(current_pdpte != new_pdpte || remap) {
        remap = true;
        pd_work = getWorkpage(2);
        if(WORKPAGE(pdpt_work)[new_pdpte].structured.present) {
            physaddr_t pd_addr = WORKPAGE(pdpt_work)[new_pdpte].structured.address << 12;
            CONTROL_PAGE[pd_work].raw = pd_addr | 0x8000000000000003;
            REFRESH_TLB;
        } else {
            physaddr_t pd_addr = palloc(1);
            WORKPAGE(pdpt_work)
            [new_pdpte].raw
                = pd_addr | 0x07;
            CONTROL_PAGE[pd_work].raw = pd_addr | 0x8000000000000003;
            REFRESH_TLB;
            memset(WORKPAGE(pd_work), 0, 4096);
        }
    }

    int pt_work;
    // Remap the PT if required
    if(current_pde != new_pde || remap) {
        pt_work = getWorkpage(3);
        if(WORKPAGE(pd_work)[new_pde].structured.present) {
            physaddr_t pt_addr = WORKPAGE(pd_work)[new_pde].structured.address << 12;
            CONTROL_PAGE[pt_work].raw = pt_addr | 0x8000000000000003;
            REFRESH_TLB;
        } else {
            physaddr_t pt_addr = palloc(1);
            WORKPAGE(pd_work)
            [new_pde].raw
                = pt_addr | 0x07;
            CONTROL_PAGE[pt_work].raw = pt_addr | 0x8000000000000003;
            REFRESH_TLB;
            memset(WORKPAGE(pt_work), 0, 4096);
        }
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
    for(int i = 0; i < MAX_WORK_PAGES; ++i) {
        if(work_pages[i] != -1) {
            releaseWorkpageIndex(work_pages[i]);
            work_pages[i] = -1;
        }
    }
    locker.unlock();
}

void Pager::enable() {
    asm volatile("mov %0, %%cr3"
                 :
                 : "a"(pml4));
}

void Pager::map(physaddr_t phys, virtaddr_t virt, size_t length, PageFlags flags) {
    ASSERT_F(locker.is_locked(), "Using an unlocked pager");
    ASSERT_F(virt != 0, "Mapping to address 0");

    bool kernel = (virt >= KERNEL_START || virt + (length << 12) > KERNEL_START) && !has_kernel_lock;
    if(kernel) kernel_locker.lock();

    virt >>= 12;
    phys >>= 12;
    int pt_work = getWorkpage(3);
    for(size_t i = 0; i < length; ++i) {
        int pml4e = ((virt + i) >> 27) & 0x1FF;
        int pdpte = ((virt + i) >> 18) & 0x1FF;
        int pde = ((virt + i) >> 9) & 0x1FF;
        int pte = ((virt + i) >> 0) & 0x1FF;

        mapStructures(pml4e, pdpte, pde);

        PageStructuresEntry& entry = WORKPAGE(pt_work)[pte];
        entry.structured.address = phys + i;
        entry.structured.present = 1;
        entry.structured.write = flags.writable;
        entry.structured.user = flags.user_accesible;
        entry.structured.execute_disable = !flags.executable;
        entry.structured.global = flags.global;
        entry.structured.cache_disabled = flags.cache_disable;
    }
    REFRESH_TLB;

    if(kernel) kernel_locker.unlock();
}

virtaddr_t Pager::kmap(physaddr_t phys, size_t length, PageFlags flags) {
    ASSERT_F(locker.is_locked(), "Using an unlocked pager");

    kernel_locker.lock();
    has_kernel_lock = true;

    virtaddr_t start = getFreeRange(first_potential_kernel_page, length);
    map(phys, start, length, flags);

    has_kernel_lock = false;
    kernel_locker.unlock();
    return start + (phys & 0xFFF);
}

physaddr_t Pager::unmap(virtaddr_t virt, size_t length) {
    ASSERT_F(locker.is_locked(), "Using an unlocked pager");

    bool kernel = (virt >= KERNEL_START || virt + (length << 12) > KERNEL_START) && !has_kernel_lock;
    if(kernel) kernel_locker.lock();

    physaddr_t addr = 0;

    virt >>= 12;
    int pt_work = getWorkpage(3);
    for(size_t i = 0; i < length; ++i) {
        int pml4e = ((virt + i) >> 27) & 0x1FF;
        int pdpte = ((virt + i) >> 18) & 0x1FF;
        int pde = ((virt + i) >> 9) & 0x1FF;
        int pte = ((virt + i) >> 0) & 0x1FF;

        mapStructures(pml4e, pdpte, pde);

        PageStructuresEntry& entry = WORKPAGE(pt_work)[pte];
        if(addr == 0) addr = entry.structured.address << 12;
        entry.raw = 0;
    }

    if(virt < first_potential_kernel_page && virt >= KERNEL_START) first_potential_kernel_page = virt;

    if(kernel) kernel_locker.unlock();
    return addr;
}

physaddr_t Pager::getPhysicalAddress(virtaddr_t virt) {
    int pml4e = ((virt) >> 39) & 0x1FF;
    int pdpte = ((virt) >> 30) & 0x1FF;
    int pde = ((virt) >> 21) & 0x1FF;
    int pte = ((virt) >> 12) & 0x1FF;

    mapStructures(pml4e, pdpte, pde);

    PageStructuresEntry& entry = WORKPAGE(getWorkpage(3))[pte];
    return (entry.structured.address << 12) | (virt & 0xFFF);
}

PageFlags Pager::getFlags(virtaddr_t virt) {
    int pml4e = ((virt) >> 39) & 0x1FF;
    int pdpte = ((virt) >> 30) & 0x1FF;
    int pde = ((virt) >> 21) & 0x1FF;
    int pte = ((virt) >> 12) & 0x1FF;

    mapStructures(pml4e, pdpte, pde);

    PageStructuresEntry& entry = WORKPAGE(getWorkpage(3))[pte];
    PageFlags flags {};
    flags.present = entry.structured.present;
    if(flags.present) {
        flags.writable = entry.structured.write;
        flags.user_accesible = entry.structured.user;
        flags.executable = !entry.structured.execute_disable;
        flags.global = entry.structured.global;
        flags.cache_disable = entry.structured.cache_disabled;
    }
    return flags;
}

virtaddr_t Pager::kalloc(size_t length) {
    ASSERT_F(locker.is_locked(), "Using an unlocked pager");

    kernel_locker.lock();
    has_kernel_lock = true;

    virtaddr_t start = getFreeRange(first_potential_kernel_page, length);
    if(start == 0) {
        has_kernel_lock = false;
        kernel_locker.unlock();
        return 0;
    }

    for(size_t i = 0; i < length; ++i) map(palloc(1), start + (i << 12), 1, PageFlags { .present = 1, .writable = 1, .user_accesible = 0, .executable = 0, .global = 1, .cache_disable = 0 });

    has_kernel_lock = false;
    kernel_locker.unlock();
    return start;
}

void Pager::free(virtaddr_t ptr, size_t length) {
    ASSERT_F(locker.is_locked(), "Using an unlocked pager");
    for(size_t i = 0; i < length; ++i) {
        physaddr_t addr = unmap(ptr + (i << 12), 1);
        pfree(addr, 1);
    }
}

virtaddr_t Pager::getFreeRange(virtaddr_t start, size_t length) {
    bool kernel = false;
    for(virtaddr_t addr = start; addr != 0; addr += 0x1000) {
        if((addr >= KERNEL_START || addr + (length << 12) > KERNEL_START) && !kernel && !has_kernel_lock) {
            kernel_locker.lock();
            kernel = true;
        }

        bool found = true;
        virtaddr_t offset;
        for(offset = 0; offset < (length << 12); offset += 0x1000) {
            if(getFlags(addr + offset).present) {
                found = false;
                break;
            }
        }
        if(found) {
            if(addr == first_potential_page) first_potential_page = addr + (length << 12);
            if(addr == first_potential_kernel_page) first_potential_kernel_page = addr + (length << 12);
            if(kernel) kernel_locker.unlock();
            return addr;
        }
        addr += offset;
    }
    if(kernel) kernel_locker.unlock();
    return 0;
}

Pager& Pager::active() {
    u64_t cr3;
    asm volatile("mov %%cr3, %0"
                 : "=a"(cr3));
    cr3 &= ~0xFFF;
    for(auto& pager : pagers) {
        if(pager->pml4 == cr3) {
            return *pager;
        }
    }
    ASSERT_NOT_REACHED("Invalid CR3 value (no pager associated)");
    while(true)
        ;
}

Pager& Pager::kernel() {
    return *kernel_pager;
}
