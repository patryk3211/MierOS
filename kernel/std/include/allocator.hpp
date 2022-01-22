#pragma once

#include <types.h>

namespace std {
    struct heap_allocator {
        template<typename T, typename... Args> T* alloc(Args... args) { return new T(args...); }
        template<typename T> T* alloc(size_t length) { return new T[length]; }
        template<typename T> void free(T* ptr) { delete ptr; }
        template<typename T> void free_array(T* ptr) { delete[] ptr; }
    };
}
