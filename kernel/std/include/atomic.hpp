#pragma once

namespace std {
    template<typename T> class Atomic {
        T value;

    public:
        template<typename... Args> Atomic(Args... args)
            : value(args...) { }
        ~Atomic() = default;

        T load();
        void store(T v);

        T fetch_add(T v);
        T fetch_sub(T v);
    };

#define _MIEROS_STANDARD_ATOMIC_HPP
#if defined(x86_64)
#include <arch/atomic.hpp>
#endif
}
