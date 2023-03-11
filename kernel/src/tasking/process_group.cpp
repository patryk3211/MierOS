#include <tasking/process_group.hpp>
#include <tasking/process.hpp>

using namespace kernel;

std::UnorderedMap<pid_t, std::WeakPtr<ProcessGroup>> ProcessGroup::s_groups;

ProcessGroup::ProcessGroup(pid_t id)
    : f_id(id) { }

ProcessGroup::~ProcessGroup() {
    s_groups.erase(f_id);
}

pid_t ProcessGroup::id() {
    return f_id;
}

void ProcessGroup::add(Process* proc) {
    f_members.push_back(proc);
}

void ProcessGroup::remove(Process* proc) {
    for(auto iter = f_members.begin(); iter != f_members.end(); ++iter) {
        if(*iter == proc) {
            f_members.erase(iter);
            return;
        }
    }
}

void ProcessGroup::signal(std::SharedPtr<siginfo_t> info) {
    for(auto& proc : f_members) {
        proc->signal(info);
    }
}

std::SharedPtr<ProcessGroup> ProcessGroup::get_group(pid_t id) {
    auto result = s_groups.at(id);
    return result ? result->shared() : nullptr;
}

std::SharedPtr<ProcessGroup> ProcessGroup::get_make_group(pid_t id) {
    auto result = s_groups.at(id);
    if(result) {
        return result->shared();
    } else {
        auto ptr = std::make_shared<ProcessGroup>(id);
        s_groups.insert({ id, ptr.weak() });
        return ptr;
    }
}

ValueOrError<void> ProcessGroup::wait(GroupSleepQueue::WaitInfo* waitInfo, pid_t parentId, bool sleep) {
    // Check if there are any child processes in this group
    bool found = false;
    for(auto& proc : f_members) {
        if(proc->ppid() == parentId) {
            found = true;
            auto state = proc->main_thread()->get_state(false);
            if(state != RUNNING) {
                // A child has exited
                waitInfo->f_waker_status = proc->state_status();
                waitInfo->f_waker_pid = proc->pid();
                return { };
            }
            break;
        }
    }

    if(!found)
        return ECHILD;

    if(sleep) {
        // Group id is the parent pid in this case since we need to wait for
        // a child process and a group can contain a non-child process (I think?)
        f_sleep_queue.sleep(waitInfo, parentId);
    } else {
        waitInfo->f_waker_pid = 0;
        waitInfo->f_waker_status = 0;
    }

    return { };
}

void ProcessGroup::notify_wait(const GroupSleepQueue::WaitInfo& info, pid_t parent) {
    f_sleep_queue.wakeup(info, parent);
}

