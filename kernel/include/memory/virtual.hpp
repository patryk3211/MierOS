#pragma once

#include <types.h>

namespace kernel {
    class Pager {
        
    public:
        Pager();
        ~Pager();

        static void init(physaddr_t kernel_base, virtaddr_t kernel_base_v);
    };
}
