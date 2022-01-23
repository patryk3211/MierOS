#pragma once

#include <stivale.h>
#include <locking/spinlock.hpp>
#include <list.hpp>

namespace kernel {
    #define MAX_WORK_PAGES 4

    struct PageFlags {
        bool present:1;
        bool writable:1;
        bool user_accesible:1;
        bool executable:1;
        bool global:1;
    };

    class Pager {
        static physaddr_t kernel_pd[2];
        static std::List<Pager*> pagers;
        static SpinLock kernel_locker;
        static virtaddr_t first_potential_kernel_page;

        physaddr_t pml4;

        int work_pages[MAX_WORK_PAGES];

        int current_pml4e;
        int current_pdpte;
        int current_pde;

        SpinLock locker;

        virtaddr_t first_potential_page;

    public:
        Pager();
        ~Pager();

        void lock();
        void unlock();

        void enable();

        void map(physaddr_t phys, virtaddr_t virt, size_t length, PageFlags flags);
        physaddr_t unmap(virtaddr_t virt, size_t length);

        virtaddr_t kmap(physaddr_t phys, size_t length, PageFlags flags);

        physaddr_t getPhysicalAddress(virtaddr_t virt);
        PageFlags getFlags(virtaddr_t virt);

        virtaddr_t kalloc(size_t length);
        void free(virtaddr_t ptr, size_t length);

        u64_t cr3() { return pml4; }

        virtaddr_t getFreeRange(virtaddr_t start, size_t length);

        static void init(physaddr_t kernel_base_p, virtaddr_t kernel_base_v, stivale2_stag_pmrs* pmrs);

        static Pager& active();
    private:
        void mapStructures(int new_pml4e, int new_pdpte, int new_pde);
        int getWorkpage(int index);

        static int aquireWorkpageIndex();
        static void releaseWorkpageIndex(int index);
    };
}
