#pragma once

#include <types.h>
#include <atomic.hpp>

namespace kernel {
    class PhysicalPage {
        physaddr_t _addr;
        std::Atomic<u32_t> _ref_count;
    public:
        PhysicalPage();
        ~PhysicalPage();

        void ref();
        void unref();
        u32_t ref_count();

        physaddr_t addr() { return _addr; }
    };
}
