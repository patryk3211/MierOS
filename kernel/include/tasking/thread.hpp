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
#include <tasking/sleep_queue.hpp>

namespace kernel {
    enum ThreadState {
        // This thread is running
        RUNNING,
        // This thread is waiting to be run
        RUNNABLE,
        // This thread is waiting for something to happen
        SLEEPING,
        // This thread is in the process of being deleted
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

        ThreadState f_state;
        SleepQueue f_state_watchers;

        Process* f_parent;

        pid_t f_pid;

    public:
        Module* f_current_module;

        Thread* f_next;

        int f_preferred_core;

        Thread(u64_t ip, bool isKernel, Process* process);
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

        pid_t pid() { return f_pid; }

        Process& parent() { return *f_parent; }
        bool is_main();

        void make_ks(virtaddr_t ip, virtaddr_t sp);

        void set_fs(virtaddr_t fs_base);
        virtaddr_t get_fs();

        void save_fpu_state();
        void load_fpu_state();

        void change_state(ThreadState newState);
        ThreadState get_state(bool sleep);

        void minimize();

        static Thread* current();
        static Thread* get(pid_t tid);

        friend class Scheduler;
        friend class Process;

    private:
        static pid_t generate_pid();
    };
}
