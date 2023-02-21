#pragma once

#include <types.h>
#include <tasking/thread.hpp>
#include <tasking/sleep_queue.hpp>

namespace kernel {
    class Futex {
        struct FutexEntry {
            physaddr_t f_addr;
            Futex* f_futex;
        };
        static std::List<FutexEntry> s_futexes;

        SleepQueue f_sleep;
        u32_t* f_addr;

        Futex(u32_t* addr);
    public:
        ~Futex();

        ValueOrError<void> wait(u32_t value);
        ValueOrError<u32_t> wake(u32_t value);

        bool empty() const;

        static Futex* get(u32_t* addr);
        static Futex* make(u32_t* addr);
    };
}

