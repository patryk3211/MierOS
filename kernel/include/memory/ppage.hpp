#pragma once

#include <atomic.hpp>
#include <memory/virtual.hpp>
#include <cstddef.hpp>
#include <types.h>

namespace kernel {
    class PhysicalPage {
        struct Data {
            physaddr_t f_addr;
            std::Atomic<u32_t> f_ref_count;
            std::Atomic<size_t> f_obj_ref_count;
            PageFlags f_flags;
            bool f_copy_on_write;
        } *data;

    public:
        PhysicalPage();
        ~PhysicalPage();

        PhysicalPage(std::nullptr_t);

        PhysicalPage(const PhysicalPage& other);
        PhysicalPage(PhysicalPage&& other);

        PhysicalPage& operator=(const PhysicalPage& other);
        PhysicalPage& operator=(PhysicalPage&& other);

        void ref();
        void unref();
        u32_t ref_count() const;

        physaddr_t addr() const;
        PageFlags& flags();
        const PageFlags& flags() const;

        bool& copy_on_write();
        const bool& copy_on_write() const;
    
    private:
        Data* leak_ptr();
    };
}
