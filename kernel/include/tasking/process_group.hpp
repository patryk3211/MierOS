#pragma once

#include <unordered_map.hpp>
#include <shared_pointer.hpp>
#include <list.hpp>
#include <asm/signal.h>
#include <tasking/sleep_queue.hpp>

namespace kernel {
    class Process;
    class ProcessGroup {
        static std::UnorderedMap<pid_t, std::WeakPtr<ProcessGroup>> s_groups;

        pid_t f_id;

        std::List<Process*> f_members;
        GroupSleepQueue f_sleep_queue;

    public:
        ProcessGroup(pid_t id);
        ~ProcessGroup();

        pid_t id();

        void add(Process* proc);
        void remove(Process* proc);

        void signal(std::SharedPtr<siginfo_t> info);

        ValueOrError<void> wait(GroupSleepQueue::WaitInfo* waitInfo, pid_t parentId, bool sleep);
        void notify_wait(const GroupSleepQueue::WaitInfo& info, pid_t parent);

        static std::SharedPtr<ProcessGroup> get_group(pid_t id);
        static std::SharedPtr<ProcessGroup> get_make_group(pid_t id);
    };
}

