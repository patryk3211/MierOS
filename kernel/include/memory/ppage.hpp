#pragma once

#include <atomic.hpp>
#include <types.h>

namespace kernel {
    class PhysicalPage {
        struct Data {
            physaddr_t f_addr;
            std::Atomic<u32_t> f_ref_count;
            std::Atomic<size_t> f_obj_ref_count;
        } *data;

    public:
        PhysicalPage();
        ~PhysicalPage();

        PhysicalPage(const PhysicalPage& other);
        PhysicalPage(PhysicalPage&& other);

        PhysicalPage& operator=(const PhysicalPage& other);
        PhysicalPage& operator=(PhysicalPage&& other);

        void ref();
        void unref();
        u32_t ref_count();

        physaddr_t addr();
    
    private:
        Data* leak_ptr();
    };
}
