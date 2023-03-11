#pragma once

#include "tasking/sleep_queue.hpp"
#include <list.hpp>
#include <streams/streamwrapper.hpp>
#include <string.hpp>
#include <tasking/thread.hpp>
#include <unordered_map.hpp>
#include <memory/ppage.hpp>
#include <tasking/syscall.h>
#include <shared_pointer.hpp>
#include <range_map.hpp>
#include <memory/page/resolved_memory.hpp>
#include <memory/page/resolvable_memory.hpp>
#include <asm/signal.h>
#include <tasking/process_group.hpp>

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

        u32_t f_status;

        std::String<> f_workingDirectory;

        fd_t f_next_fd;
        std::UnorderedMap<fd_t, StreamWrapper> f_streams;

        struct Signal {
            std::SharedPtr<siginfo_t> info;
            pid_t deliver_to;
        };

        SpinLock f_signal_lock;
        std::Vector<SignalAction> f_signal_actions;
        std::List<Signal> f_signal_queue;

        uid_t f_uid;
        uid_t f_euid;
        gid_t f_gid;
        gid_t f_egid;

        Process* f_parent;
        std::SharedPtr<ProcessGroup> f_group;
        std::List<Process*> f_children;

        VNodePtr f_terminal;

        virtaddr_t f_first_free;
        std::UnorderedMap<virtaddr_t, ResolvedMemoryEntry> f_resolved_memory;
        std::RangeMap<virtaddr_t, std::SharedPtr<ResolvableMemoryEntry>> f_resolvable_memory;

        GroupSleepQueue f_children_wait;

        RecursiveMutex f_lock;

        Process(Pager* pager);
        Process(virtaddr_t entry_point, Pager* kern_pager);

        bool minimize();
        void reset_signals();
    public:
        Process(virtaddr_t entry_point);
        ~Process() = default;

        uid_t uid();
        uid_t euid();
        gid_t gid();
        gid_t egid();

        void die(u32_t exitCode);

        Thread* main_thread() { return f_threads.front(); }
        Pager& pager() { return *f_pager; }

        pid_t pid();
        pid_t ppid();
        pid_t pgid();

        std::String<>& cwd() { return f_workingDirectory; }
        VNodePtr& ctty() { return f_terminal; }

        u32_t state_status() { return f_status; }

        void signal_lock();
        void signal_unlock();
        SignalAction& action(int sigNum);
        void signal(std::SharedPtr<siginfo_t> sig, pid_t threadDelivery = 0);
        
        void handle_signal(Thread* onThread);

        fd_t add_stream(Stream* stream);
        ValueOrError<void> close_stream(fd_t fd);
        Stream* get_stream(fd_t fd);
        ValueOrError<fd_t> dup_stream(fd_t oldFd, fd_t newFd, int flags);
        ValueOrError<StreamWrapper&> get_stream_wrapper(fd_t fd);

        Process* fork();
        ValueOrError<void> execve(const VNodePtr& file, char* argv[], char* envp[]);

        virtaddr_t get_free_addr(virtaddr_t hint, size_t pageLength);
        void map_page(virtaddr_t addr, PhysicalPage& page, bool shared, bool copyOnWrite);
        void alloc_pages(virtaddr_t addr, size_t pageLength, int flags, int prot);
        void null_pages(virtaddr_t addr, size_t pageLength);
        void file_pages(virtaddr_t addr, size_t byteLength, VNodePtr file, size_t fileOffset, bool shared, bool write, bool execute);
        void unmap_pages(virtaddr_t addr, size_t pageLength);

        bool handle_page_fault(virtaddr_t fault_address, u32_t code);

        ProcessGroup& group();
        ValueOrError<void> set_group(pid_t id);

        ValueOrError<void> wait_for_child(GroupSleepQueue::WaitInfo* info, bool sleep);
        void notify_state_change(ThreadState state);

        static Process* construct_kernel_process(virtaddr_t entry_point);
        static void cleanup_process(Process* proc);

        friend class Scheduler;
    };
}
