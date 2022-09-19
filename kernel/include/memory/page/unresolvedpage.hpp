#pragma once

#include <types.h>
#include <memory/ppage.hpp>

namespace kernel {
    class UnresolvedPage {
    public:
        virtual PhysicalPage resolve(virtaddr_t addr);
    };
}
