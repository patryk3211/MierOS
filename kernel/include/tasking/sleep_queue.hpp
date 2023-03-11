#pragma once

#include <locking/semaphore.hpp>
#include <queue.hpp>
#include <errno.h>
#include <list.hpp>

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

    class GroupSleepQueue {
    public:
        struct WaitInfo {
            pid_t f_waker_pid;
            int f_waker_status;
        };

    private:
        struct QueueEntry {
            pid_t f_thread_id;
            WaitInfo* f_pass_argument;
            pid_t f_group_id;
        };

        std::List<QueueEntry> f_sleeping;
        Semaphore f_access;

    public:
        GroupSleepQueue();
        ~GroupSleepQueue();

        ValueOrError<void> sleep(WaitInfo* storeArg, pid_t groupId);
        ValueOrError<size_t> wakeup(const WaitInfo& arg, pid_t groupId, size_t count = -1);

        bool empty() const;
    };
}

