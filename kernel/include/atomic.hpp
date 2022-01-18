#pragma once

//#include <stdatomic.h>

namespace kernel {
    template<typename T> class Atomic {
        T value;
    public:
        template<typename... Args> Atomic(Args... args) : value(args...) { }

        ~Atomic() = default;

        T load() {
            //return atomic_load(&value);
            return value;
        }

        void store(T v) {
            //atomic_store(&value, v);
            value = v;
        }
    
        T fetch_add(T v) {
            //return atomic_fetch_add(&value, v);
            T t = value;
            value += v;
            return t;
        }

        T fetch_sub(T v) {
            //return atomic_fetch_sub(&value, v);
            T t = value;
            value -= v;
            return t;
        }
    };
}
