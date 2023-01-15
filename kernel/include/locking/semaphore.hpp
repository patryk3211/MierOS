#pragma once

#include <types.h>
#include <stdatomic.h>

namespace kernel {
    class Semaphore {
        atomic_int f_current;
        atomic_int f_max;

    public:
        Semaphore(atomic_int amount);
        Semaphore(atomic_int amount, atomic_int initial);
        ~Semaphore();

        void acquire();
        void release();
    };
}
