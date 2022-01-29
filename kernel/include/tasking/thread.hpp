#pragma once

#include <types.h>
#include <tasking/cpu_state.h>
#include <function.hpp>
#include <memory/kbuffer.hpp>
#include <locking/spinlock.hpp>
#include <list.hpp>
#include <memory/virtual.hpp>
#include <unordered_map.hpp>
#include <modules/module.hpp>

namespace kernel {
    enum ThreadState {
        RUNNING,
        RUNNABLE,
        SLEEPING
    };

    class Process;
    class Thread {
        static pid_t next_pid;
        static std::UnorderedMap<pid_t, Thread*> threads;
        static SpinLock pid_lock;

        KBuffer kernel_stack;
        // Kernel Stack Pointer
        CPUState* ksp;

        SpinLock lock;

        std::List<std::Function<bool()>> blockers;

        ThreadState state;

        Process& parent;

        pid_t _pid;
    public:
        Module* current_module;

        Thread* next;

        int preferred_core;
    
        Thread(u64_t ip, bool isKernel, Process& process);
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

        pid_t pid() { return _pid; }

        static Thread* current();

        friend class Scheduler;
        friend class Process;
    
    private:
        static pid_t generate_pid();
    };
}
