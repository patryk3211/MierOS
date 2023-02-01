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
        KBuffer f_fpu_state;
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
         * @brief Put the thread to sleep.
         *
         * This function will transition a thread into the SLEEPING state.
         * If this thread is the current thread it will force a task switch and the
         * thread will not execute until it is put back into the running queue.
         */
        void sleep(bool schedule = true);

        /**
         * @brief Wake thread up from sleep.
         *
         * This function will transition a thread into the RUNNABLE state
         * from the SLEEPING state. It will also put it back on the scheduler queue.
         */
        void wakeup();

        void schedule_finalization();

        pid_t pid() { return f_pid; }

        Process& parent() { return f_parent; }

        void make_ks(virtaddr_t ip, virtaddr_t sp);

        void set_fs(virtaddr_t fs_base);
        virtaddr_t get_fs();

        void save_fpu_state();
        void load_fpu_state();

        static Thread* current();

        friend class Scheduler;
        friend class Process;

    private:
        static pid_t generate_pid();
    };
}
