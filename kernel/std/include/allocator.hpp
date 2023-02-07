#pragma once

#include <types.h>
#include <dmesg.h>

namespace std {
    struct heap_allocator {
        template<typename T, typename... Args> T* alloc(Args... args) { return new T(args...); }
        template<typename T> T* alloc(size_t length) { return new T[length]; }
        template<typename T> void free(T* ptr) { delete ptr; }
        template<typename T> void free_array(T* ptr) { delete[] ptr; }
    };

    struct traced_heap_allocator {
        template<typename T, typename... Args> T* alloc(Args... args) {
            auto* ptr = new T(args...);
            TRACE("Allocated new ptr: 0x%x16\n", ptr);
            return ptr;
        }
        template<typename T> T* alloc(size_t length) {
            auto* ptr = new T[length];
            TRACE("Allocated new array of length %d: 0x%x16\n", length, ptr);
            return ptr;
        }
        template<typename T> void free(T* ptr) {
            delete ptr;
            TRACE("Freed 0x%x16 ptr\n", ptr);
        }
        template<typename T> void free_array(T* ptr) {
            delete[] ptr;
            TRACE("Freed 0x%x16 array\n", ptr);
        }
    };
}
