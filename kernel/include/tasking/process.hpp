#pragma once

#include <list.hpp>
#include <streams/streamwrapper.hpp>
#include <string.hpp>
#include <tasking/thread.hpp>
#include <unordered_map.hpp>
#include <memory/ppage.hpp>
#include <tasking/syscall.h>
#include <shared_pointer.hpp>
#include <memory/page/filepage.hpp>

#define __KERNEL__
#include <asm/signal.h>

namespace kernel {
    struct SignalAction {
        void (*handler)(...);
        int flags;
        sigmask_t mask;
        void (*restorer)();
    }; 

    class Process {
        std::List<Thread*> f_threads;

        Pager* f_pager;

        u8_t f_exitStatus;
        bool f_signalled;

        std::String<> f_workingDirectory;

        fd_t f_next_fd;
        std::UnorderedMap<fd_t, StreamWrapper> f_streams;

        struct Signal {
            siginfo_t* info;
            pid_t deliver_to;
        };

        SpinLock f_signal_lock;
        std::Vector<SignalAction> f_signal_actions;
        std::List<Signal> f_signal_queue;

        uid_t f_uid;
        uid_t f_euid;
        gid_t f_gid;
        gid_t f_egid;

        pid_t f_parent;

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
        std::UnorderedMap<virtaddr_t, std::SharedPtr<MemoryEntry>> f_memorymap;

        RecursiveMutex f_lock;

        Process(Pager* pager);
        Process(virtaddr_t entry_point, Pager* kern_pager);

        void set_page_mapping(virtaddr_t addr, std::SharedPtr<MemoryEntry>& entry);

        bool minimize();
        void reset_signals();
    public:
        Process(virtaddr_t entry_point);
        ~Process();

        uid_t uid();
        uid_t euid();
        gid_t gid();
        gid_t egid();

        void die(u32_t exitCode);

        Thread* main_thread() { return f_threads.front(); }
        Pager& pager() { return *f_pager; }

        pid_t pid();
        pid_t ppid();

        std::String<>& cwd() { return f_workingDirectory; }

        bool signalled() { return f_signalled; }
        u8_t exit_status() { return f_exitStatus; }

        void signal_lock();
        void signal_unlock();
        SignalAction& action(int sigNum);
        void signal(siginfo_t* info, pid_t threadDelivery = 0);
        
        void handle_signal(Thread* onThread);

        fd_t add_stream(Stream* stream, fd_t hint = -1);
        void close_stream(fd_t fd);
        Stream* get_stream(fd_t fd);
        ValueOrError<fd_t> dup_stream(fd_t oldFd, fd_t newFd);

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
