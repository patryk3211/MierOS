#pragma once

#include <assert.h>

namespace kernel {
    class SpinLock {
        u32_t locked;

    public:
        SpinLock() {
            locked = false;
        }

        ~SpinLock() {
            locked = false;
        }

        void lock() {
            asm volatile(
                "mov $1, %%ecx\n"
                "xor %%eax, %%eax\n"
                "_SpinLock.lock_try_lock:\n"
                "lock cmpxchg %%ecx, %0\n"
                "je _SpinLock.lock_out\n"
                "_SpinLock.lock_pause:\n"
                "mov %0, %%eax\n"
                "test %%eax, %%eax\n"
                "jz _SpinLock.lock_try_lock\n"
                "pause\n"
                "jmp _SpinLock.lock_pause\n"
                "_SpinLock.lock_out:"
                :
                : "m"(locked)
                : "eax", "ecx");
        }

        void unlock() {
            ASSERT_F(locked, "Unlocking an unlocked lock");
            asm volatile("" ::
                             : "memory");
            locked = false;
        }

        bool try_lock() {
            bool has_locked = false;
            asm volatile(
                "mov $1, %%ecx\n"
                "xor %%eax, %%eax\n"
                "lock cmpxchg %%ecx, %0\n"
                "jne _SpinLock.try_lock_out\n"
                "movb $1, %1\n"
                "_SpinLock.try_lock_out:"
                :
                : "m"(locked), "m"(has_locked)
                : "eax", "ecx");
            return has_locked;
        }

        bool is_locked() const {
            return locked == 1;
        }
    };
}
