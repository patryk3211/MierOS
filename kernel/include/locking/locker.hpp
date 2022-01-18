#pragma once

namespace kernel {
    template <class Lock> class Locker {
        Lock& lock;

    public:
        Locker(Lock& lock) : lock(lock) {
            lock.lock();
        }

        ~Locker() {
            lock.unlock();
        }
    };
}
