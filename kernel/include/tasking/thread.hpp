#pragma once

#include <types.h>
#include <tasking/cpu_state.h>
#include <function.hpp>
#include <memory/kbuffer.hpp>
#include <locking/spinlock.hpp>
#include <list.hpp>
#include <memory/virtual.hpp>

namespace kernel {
    enum ThreadState {
        RUNNING,
        RUNNABLE,
        SLEEPING
    };

    class Thread {
        KBuffer kernel_stack;
        // Kernel Stack Pointer
        CPUState* ksp;

        SpinLock lock;

        std::List<std::Function<bool()>> blockers;

        ThreadState state;
    public:
        Thread* next;

        int preferred_core;
    
        Thread(u64_t ip, bool isKernel, Pager& pager);
        ~Thread();

        /**
         * @brief Puts the thread into SLEEPING state.
         *
         * @param until Wakes the thread up when true is returned.
         **/
        void sleep(std::Function<bool()>& until);

        /**
         * @brief Transitions the thread back into RUNNABLE state.
         * 
         * @return true - The thread is now RUNNABLE,
         *         false - The thread is still SLEEPING
         */
        bool try_wakeup();

        static Thread* current();
    };
}
