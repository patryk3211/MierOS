#pragma once

#include <list.hpp>
#include <streams/stream.hpp>
#include <string.hpp>
#include <tasking/thread.hpp>
#include <unordered_map.hpp>
#include <memory/ppage.hpp>
#include <tasking/syscall.h>
#include <shared_pointer.hpp>
#include <memory/page/filepage.hpp>

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
                MEMORY, ANONYMOUS, FILE, FILE_MEMORY, EMPTY
            } type;
            void* page;
            bool shared;

            ~MemoryEntry();
        };

        virtaddr_t f_first_free;
        /// TODO: [07.02.2023] Change from UnorderedMap to RangeMap (to be implemented)
        std::UnorderedMap<virtaddr_t, std::SharedPtr<MemoryEntry>, std::hash<virtaddr_t>, std::equal_to<virtaddr_t>, std::traced_heap_allocator> f_memorymap;

        RecursiveMutex f_lock;

        Process(virtaddr_t entry_point, Pager* kern_pager);

        void set_page_mapping(virtaddr_t addr, std::SharedPtr<MemoryEntry>& entry);

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
        ValueOrError<void> execve(const VNodePtr& file, char* argv[], char* envp[]);

        virtaddr_t get_free_addr(virtaddr_t hint, size_t length);
        void map_page(virtaddr_t addr, PhysicalPage& page, bool shared);
        void alloc_pages(virtaddr_t addr, size_t length, int flags, int prot);
        void null_pages(virtaddr_t addr, size_t length);
        void file_pages(virtaddr_t addr, size_t length, FilePage* page);
        void unmap_pages(virtaddr_t addr, size_t length);

        bool handle_page_fault(virtaddr_t fault_address, u32_t code);

        static Process* construct_kernel_process(virtaddr_t entry_point);

        friend class Scheduler;
    };
}
