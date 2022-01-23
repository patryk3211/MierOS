#pragma once

#include <tasking/thread_queue.hpp>
#include <tasking/cpu_state.h>

namespace kernel {
    class Scheduler {
        static ThreadQueue wait_queue;
        static ThreadQueue runnable_queue;
        static Scheduler* schedulers;

        int core_id;
        Thread* current_thread;

        KBuffer idle_stack;
        CPUState* idle_ksp;
    public:
        Scheduler();
        ~Scheduler();

        static void init(int core_count);
        static Scheduler& scheduler(int core);
    private:
        static void idle();
    };
}
