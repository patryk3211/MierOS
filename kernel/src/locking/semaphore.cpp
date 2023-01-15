#include <locking/semaphore.hpp>

#include <assert.h>

using namespace kernel;

Semaphore::Semaphore(atomic_int amount) {
    f_max = amount;
    f_current = amount;
}

Semaphore::Semaphore(atomic_int amount, atomic_int initial) {
    f_max = amount;
    f_current = initial;
}

Semaphore::~Semaphore() {
    f_current = f_max;
}

void Semaphore::acquire() {
    while(true) {
        while(atomic_load_explicit(&f_current, memory_order_acquire) <= 0)
            asm volatile("rep nop");
        int tmp = atomic_fetch_add_explicit(&f_current, -1, memory_order_acq_rel);
        if(tmp > 0) break;
        else atomic_fetch_add_explicit(&f_current, 1, memory_order_release);
    }
}

void Semaphore::release() {
    auto val = atomic_fetch_add_explicit(&f_current, 1, memory_order_release);
    ASSERT_F(val < f_max, "Releasing the semaphore more than the max use count");
}
