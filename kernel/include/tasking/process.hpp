#pragma once

#include <list.hpp>
#include <streams/stream.hpp>
#include <string.hpp>
#include <tasking/thread.hpp>
#include <unordered_map.hpp>
#include <memory/ppage.hpp>
#include <tasking/syscall.h>

namespace kernel {
    class Process {
        std::List<Thread*> f_threads;

        Pager* f_pager;

        u32_t f_exitCode;

        std::String<> f_workingDirectory;

        fd_t f_next_fd;
        std::UnorderedMap<fd_t, Stream*> f_streams;

        struct MemoryEntry {
            PhysicalPage page;
        };

        std::UnorderedMap<virtaddr_t, MemoryEntry> f_memorymap;

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

        Process* fork();

        void map_page(virtaddr_t addr, PhysicalPage& page);
        PhysicalPage get_page(virtaddr_t addr);

        static Process* construct_kernel_process(virtaddr_t entry_point);

        friend class Scheduler;
    };
}
