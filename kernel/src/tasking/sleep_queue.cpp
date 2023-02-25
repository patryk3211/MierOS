#include <tasking/sleep_queue.hpp>
#include <arch/interrupts.h>
#include <tasking/thread.hpp>
#include <assert.h>

using namespace kernel;

SleepQueue::SleepQueue()
    : f_access(1, 1) {
}

SleepQueue::~SleepQueue() {
    ASSERT_F(f_sleeping.size() == 0, "Deleting a sleep queue without waking up all the threads");
}

ValueOrError<void> SleepQueue::sleep() {
    // Treat this whole operation as critical. This will guarantee that
    // we complete it and release a lock before making a task switch and
    // later on we won't end up in a deadlock situation when we try to
    // wake up the waiters as a dying task.
    enter_critical();

    // The semaphore guarantees atomicity on multiprocessor systems.
    f_access.acquire();
    auto* thread = Thread::current();

    thread->sleep(false);
    f_sleeping.push_back(thread->pid());
    f_access.release();
    leave_critical();

    force_task_switch();
    return { };
}

ValueOrError<size_t> SleepQueue::wakeup(size_t count) {
    f_access.acquire();

    size_t woke = 0;
    while(woke++ < count && f_sleeping.size() > 0) {
        auto tid = f_sleeping.pop_front();
        auto* thread = Thread::get(tid);
        TRACE("(SleepQueue) Waking up thread (tid = %d), thread ptr = 0x%016lx", tid, thread);
        if(thread) thread->wakeup();
    }

    f_access.release();
    return woke;
}

bool SleepQueue::empty() const {
    return f_sleeping.size() == 0;
}

