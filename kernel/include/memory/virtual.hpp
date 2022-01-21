#pragma once

#include <stivale.h>
#include <locking/spinlock.hpp>

namespace kernel {
    struct PageFlags {
        bool present:1;
        bool writable:1;
        bool user_accesible:1;
    };

    class Pager {
        static physaddr_t kernel_pd[2];

        physaddr_t pml4;

        int work_pages[6];

        int current_pml4e;
        int current_pdpte;
        int current_pde;

        SpinLock locker;

    public:
        Pager();
        ~Pager();

        void lock();
        void unlock();

        void enable();

        static void init(physaddr_t kernel_base_p, virtaddr_t kernel_base_v, stivale2_stag_pmrs* pmrs);
    private:
        void mapStructures(int new_pml4e, int new_pdpte, int new_pde);
        int getWorkpage(int index);

        static int aquireWorkpageIndex();
        static void releaseWorkpageIndex(int index);
    };
}
