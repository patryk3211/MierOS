#pragma once

#include <assert.h>

namespace kernel {
    class SpinLock {
        bool locked;
    public:
        SpinLock() {
            locked = false;
        }

        ~SpinLock() {
            locked = false;
        }

        void lock() {
            while(locked);
            locked = true;
        }

        void unlock() {
            ASSERT_F(locked, "Unlocking an unlocked lock");
            locked = false;
        }

        bool is_locked() const {
            return locked;
        }
    };
}
