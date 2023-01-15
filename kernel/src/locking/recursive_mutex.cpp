#include <locking/recursive_mutex.hpp>

#include <tasking/thread.hpp>
#include <assert.h>

using namespace kernel;

RecursiveMutex::RecursiveMutex() : f_semaphore(1, 0) {
    f_counter = 0;
    f_recursion = 0;
    f_owner = 0;
}

RecursiveMutex::~RecursiveMutex() { }

void RecursiveMutex::lock() {
    auto* t = Thread::current();
    pid_t tid = t != 0 ? t->pid() : 1;

    if(atomic_fetch_add(&f_counter, 1) > 0) {
        if(tid != f_owner) f_semaphore.acquire();
    }

    f_owner = tid;
    ++f_recursion;
}

void RecursiveMutex::unlock() {
    auto* t = Thread::current();
    pid_t tid = t != 0 ? t->pid() : 1;
    ASSERT_F(tid == f_owner, "You are not the owner of this lock");

    u32_t r = --f_recursion;
    if(r == 0) f_owner = 0;

    if(atomic_fetch_add(&f_counter, -1) > 1) {
        if(r == 0) f_semaphore.release();
    }
}

bool RecursiveMutex::try_lock() {
    auto* t = Thread::current();
    pid_t tid = t != 0 ? t->pid() : 1;
    if(f_owner == tid) {
        atomic_fetch_add(&f_counter, 1);
    } else {
        int expected = 0;
        if(atomic_compare_exchange_strong(&f_counter, &expected, 1)) {
            f_owner = tid;
        } else return false;
    }

    ++f_recursion;
    return true;
}

bool RecursiveMutex::is_locked() const {
    return f_counter > 0;
}
