#pragma once

#include <list.hpp>
#include <tasking/thread.hpp>

namespace kernel {
    class Process {
        std::List<Thread*> f_threads;

        Pager* f_pager;

        u32_t f_exitCode;

        Process(virtaddr_t entry_point, Pager* kern_pager);

    public:
        Process(virtaddr_t entry_point);
        ~Process();

        void die(u32_t exitCode);

        Thread* mainThread() { return f_threads.front(); }
        Pager& pager() { return *f_pager; }

        static Process* construct_kernel_process(virtaddr_t entry_point);

        friend class Scheduler;
    };
}
