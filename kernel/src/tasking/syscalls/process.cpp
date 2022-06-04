#include <tasking/syscall.h>

using namespace kernel;

syscall_arg_t syscall_exit(Process& proc, syscall_arg_t exitCode) {
    proc.die(exitCode);
    return 0;
}

syscall_arg_t syscall_fork(Process& proc) {
    Process* child = proc.fork();

    return child->main_thread()->pid();
}

Process* Process::fork() {
    f_lock.lock();
    Thread* caller = Thread::current();

    // Create a child process and clone the CPU state
    Process* child = new Process(CPUSTATE_IP(caller->f_ksp));
    memcpy(child->main_thread()->f_ksp, main_thread()->f_ksp, sizeof(CPUState));
    CPUSTATE_RET(child->main_thread()->f_ksp) = 0;

    f_pager->lock();
    for(auto entry : f_memorymap) {
        if(entry.value.page.addr() != 0) { // Memory page
            // Set all writable pages as copy-on-write
            if(entry.value.page.flags().writable) {
                entry.value.page.flags().writable = false;
                entry.value.page.copy_on_write() = true;
                f_pager->map(entry.value.page.addr(), entry.key, 1, entry.value.page.flags());
            }

            // Map page to child
            child->map_page(entry.key, entry.value.page);
        } else { // Unresolved page
            /// TODO: [04.06.2022] Handle cloning of file mappings
        }
    }
    f_pager->unlock();

    // Clone streams
    child->f_next_fd = f_next_fd;
    for(auto entry : f_streams) {
        child->f_streams.insert({ entry.key, entry.value });
    }


    f_lock.unlock();

    return child;
}
