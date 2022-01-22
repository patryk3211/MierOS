#pragma once

#include <types.h>
#include <atomic.hpp>

namespace kernel {
    class KBuffer {
        void* raw_ptr;
        size_t page_size;
        std::Atomic<u32_t>* ref_count;
    public:
        KBuffer(size_t size);
        ~KBuffer();

        KBuffer(KBuffer&& other);
        KBuffer& operator=(KBuffer&& other);

        KBuffer(const KBuffer& other);
        KBuffer& operator=(const KBuffer& other);

        template<typename T = void> T* ptr() { return static_cast<T*>(raw_ptr); }
        template<typename T = void> const T* ptr() const { return static_cast<const T*>(raw_ptr); }
    };
}
