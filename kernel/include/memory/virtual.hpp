#pragma once

#include <stivale.h>

namespace kernel {
    class Pager {
        
    public:
        Pager();
        ~Pager();

        static void init(physaddr_t kernel_base_p, virtaddr_t kernel_base_v, stivale2_stag_pmrs* pmrs);
    };
}
