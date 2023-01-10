#pragma once

#include <function.hpp>
#include <list.hpp>
#include <locking/spinlock.hpp>
#include <memory/kbuffer.hpp>
#include <memory/virtual.hpp>
#include <modules/module.hpp>
#include <tasking/cpu_state.h>
#include <types.h>
#include <unordered_map.hpp>

namespace kernel {
    enum ThreadState {
        // This thread is running
        RUNNING,
        // This thread is waiting to be run
        RUNNABLE,
        // This thread is waiting for something to happen
        SLEEPING,
        // This thread is in the process of being deleted, a different task may be scheduled to check it's state
        DYING,
        // This thread has been deleted but not finalized yet
        DEAD
    };

    class Process;
    class Thread {
        static pid_t s_next_pid;
        static std::UnorderedMap<pid_t, Thread*> s_threads;
        static std::List<pid_t> s_finalizable_threads;
        static SpinLock s_pid_lock;

        KBuffer f_kernel_stack;
        // Kernel Stack Pointer
        CPUState* f_ksp;

        CPUState* f_syscall_state;

        SpinLock f_lock;

        std::List<std::Function<bool()>> f_blockers;

        ThreadState f_state;

        Process& f_parent;

        pid_t f_pid;

    public:
        Module* f_current_module;

        Thread* f_next;

        int f_preferred_core;

        bool f_watched;

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

        void schedule_finalization();

        pid_t pid() { return f_pid; }

        Process& parent() { return f_parent; }

        void make_ks(virtaddr_t ip, virtaddr_t sp);

        static Thread* current();

        friend class Scheduler;
        friend class Process;

    private:
        static pid_t generate_pid();
    };
}
