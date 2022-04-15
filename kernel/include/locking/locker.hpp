#pragma once

namespace kernel {
    template<class Lockable> class Locker {
        Lockable& lock;

    public:
        Locker(Lockable& lock)
            : lock(lock) {
            lock.lock();
        }

        ~Locker() {
            lock.unlock();
        }
    };
}
