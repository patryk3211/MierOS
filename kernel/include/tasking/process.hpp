#pragma once

#include <list.hpp>
#include <string.hpp>
#include <tasking/thread.hpp>
#include <unordered_map.hpp>
#include <streams/stream.hpp>

namespace kernel {
    class Process {
        std::List<Thread*> f_threads;

        Pager* f_pager;

        u32_t f_exitCode;

        std::String<> f_workingDirectory;

        fd_t f_next_fd;
        std::UnorderedMap<fd_t, Stream*> f_streams;

        SpinLock f_lock;

        Process(virtaddr_t entry_point, Pager* kern_pager);

    public:
        Process(virtaddr_t entry_point);
        ~Process();

        void die(u32_t exitCode);

        Thread* main_thread() { return f_threads.front(); }
        Pager& pager() { return *f_pager; }

        std::String<>& cwd() { return f_workingDirectory; }

        fd_t add_stream(Stream* stream);
        void close_stream(fd_t fd);
        Stream* get_stream(fd_t fd);

        static Process* construct_kernel_process(virtaddr_t entry_point);

        friend class Scheduler;
    };
}
