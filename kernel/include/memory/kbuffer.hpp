#pragma once

#include <types.h>
#include <atomic.hpp>
#include <utility.hpp>

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

    template<typename T> class TypedKBuffer {
        KBuffer buffer;
    public:
        TypedKBuffer() : buffer(sizeof(T)) { }
        TypedKBuffer(size_t element_count) : buffer(sizeof(T) * element_count) { }

        TypedKBuffer(TypedKBuffer<T>&& other) : buffer(std::move(other.buffer)) { }
        TypedKBuffer<T>& operator=(TypedKBuffer<T>&& other) {
            buffer = std::move(other.buffer);
        }

        TypedKBuffer(const TypedKBuffer<T>& other) : buffer(other.buffer) { }
        TypedKBuffer<T>& operator=(const TypedKBuffer<T>& other) {
            buffer = other.buffer;
        }

        T* ptr() { return buffer.ptr<T>(); }
        const T* ptr() const { return buffer.ptr<T>(); }
    
        T* operator->() { return buffer.ptr<T>(); }
        const T* operator->() const { return buffer.ptr<T>(); }

        T& operator*() { return *buffer.ptr<T>(); }
        const T& operator*() const { return *buffer.ptr<T>(); }

        T& operator[](size_t index) { return buffer.ptr<T>()[index]; }
        const T& operator[](size_t index) const { return buffer.ptr<T>()[index]; }
    };
}
