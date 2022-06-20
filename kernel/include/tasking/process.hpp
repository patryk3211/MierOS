#pragma once

#include <list.hpp>
#include <streams/stream.hpp>
#include <string.hpp>
#include <tasking/thread.hpp>
#include <unordered_map.hpp>
#include <memory/ppage.hpp>
#include <tasking/syscall.h>
#include <shared_pointer.hpp>

namespace kernel {
    class Process {
        std::List<Thread*> f_threads;

        Pager* f_pager;

        u32_t f_exitCode;

        std::String<> f_workingDirectory;

        fd_t f_next_fd;
        std::UnorderedMap<fd_t, Stream*> f_streams;

        struct MemoryEntry {
            enum Type {
                MEMORY, ANONYMOUS, FILE
            } type;
            void* page;
            bool shared;

            ~MemoryEntry();
        };

        std::UnorderedMap<virtaddr_t, std::SharedPtr<MemoryEntry>> f_memorymap;

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
        void alloc_pages(virtaddr_t addr, size_t length, int flags, int prot);

        void handle_page_fault(virtaddr_t fault_address, u32_t code);

        static Process* construct_kernel_process(virtaddr_t entry_point);

        friend class Scheduler;
    };
}
