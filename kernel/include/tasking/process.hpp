#pragma once

#include <list.hpp>
#include <tasking/thread.hpp>

namespace kernel {
    class Process {
        std::List<Thread*> threads;

        Pager* _pager;

        Process(virtaddr_t entry_point, Pager* kern_pager);
    public:
        Process(virtaddr_t entry_point);
        ~Process();

        Pager& pager() { return *_pager; }

        static Process* construct_kernel_process(virtaddr_t entry_point);

        friend class Scheduler;
    };
}
