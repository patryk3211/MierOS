#pragma once

#include <locking/semaphore.hpp>
#include <queue.hpp>
#include <errno.h>

namespace kernel {
    class SleepQueue {
        // We refer to threads by ids in case they decide to die in their sleep.
        std::Queue<pid_t> f_sleeping;
        Semaphore f_access;

    public:
        SleepQueue();
        ~SleepQueue();

        ValueOrError<void> sleep();
        ValueOrError<size_t> wakeup(size_t count = -1);

        bool empty() const;
    };
}

