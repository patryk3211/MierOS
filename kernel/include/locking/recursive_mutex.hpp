#pragma once

#include <types.h>
#include <locking/semaphore.hpp>

namespace kernel {
    class RecursiveMutex {
        atomic_int f_counter;
        u32_t f_recursion;
        pid_t f_owner;
        Semaphore f_semaphore;

    public:
        RecursiveMutex();
        ~RecursiveMutex();

        void lock();
        void unlock();

        bool try_lock();

        bool is_locked() const;
    };
}
