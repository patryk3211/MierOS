#pragma once

#include <tasking/thread_queue.hpp>
#include <tasking/cpu_state.h>

#include <tasking/process.hpp>

namespace kernel {
    class Scheduler {
        static ThreadQueue wait_queue;
        static ThreadQueue runnable_queue;
        static Scheduler* schedulers;
        static SpinLock queue_lock;

        int core_id;
        Thread* current_thread;

        KBuffer idle_stack;
        CPUState* idle_ksp;

        bool is_idle;
    public:
        Scheduler();
        ~Scheduler();

        CPUState* schedule(CPUState* current_state);

        Thread* thread() { return current_thread; }

        static void init(int core_count);
        static Scheduler& scheduler(int core);

        static void schedule_process(Process& proc);
    private:
        static void idle();
    };
}
