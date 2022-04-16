#pragma once

#include <tasking/cpu_state.h>
#include <tasking/thread_queue.hpp>

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

        bool _is_idle     : 1;
        bool first_switch : 1;

    public:
        Scheduler();
        ~Scheduler();

        CPUState* schedule(CPUState* current_state);

        Thread* thread() { return current_thread; }

        static void init(int core_count);
        static Scheduler& scheduler(int core);

        static void schedule_process(Process& proc);
        static void remove_thread(Thread* thread);

        bool is_idle() { return _is_idle; }

    private:
        static void idle();
    };
}
