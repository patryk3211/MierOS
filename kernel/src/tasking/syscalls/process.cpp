#include <tasking/syscall.h>
#include <tasking/scheduler.hpp>

using namespace kernel;

syscall_arg_t syscall_exit(Process& proc, syscall_arg_t exitCode) {
    proc.die(exitCode);
    return 0;
}

syscall_arg_t syscall_fork(Process& proc) {
    Process* child = proc.fork();

    Scheduler::schedule_process(*child);

    return child->main_thread()->pid();
}

Process* Process::fork() {
    f_lock.lock();
    Thread* caller = Thread::current();

    // Create a child process and clone the CPU state
    Process* child = new Process(CPUSTATE_IP(caller->f_syscall_state));
    memcpy(child->main_thread()->f_ksp, caller->f_syscall_state, sizeof(CPUState));
    CPUSTATE_RET(child->main_thread()->f_ksp) = 0;

    // Copy the memory space
    f_pager->lock();
    for(auto entry : f_memorymap) {
        switch(entry.value->type) {
            case MemoryEntry::MEMORY: {
                // Set all writable pages as copy-on-write
                PhysicalPage& page = *(PhysicalPage*)entry.value->page;
                if(page.flags().writable && !entry.value->shared) {
                    page.flags().writable = false;
                    page.copy_on_write() = true;
                    f_pager->map(page.addr(), entry.key, 1, page.flags());
                }

                // Map page to child
                child->map_page(entry.key, page, entry.value->shared);
                break;
            }
            case MemoryEntry::EMPTY:
            case MemoryEntry::ANONYMOUS:
                // Copy the memory entries
                child->f_memorymap[entry.key] = entry.value;
                break;
        }
    }
    f_pager->unlock();

    // Clone streams
    child->f_next_fd = f_next_fd;
    for(auto entry : f_streams) {
        child->f_streams.insert({ entry.key, entry.value });
    }

    // Copy other properties of the process
    child->f_workingDirectory = f_workingDirectory;

    f_lock.unlock();

    return child;
}
