#pragma once

#include <atomic.hpp>
#include <types.h>

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
