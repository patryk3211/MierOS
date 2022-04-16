#include <arch/interrupts.h>
#include <errno.h>
#include <tasking/process.hpp>
#include <tasking/scheduler.hpp>

using namespace kernel;

Process::Process(virtaddr_t entry_point, Pager* pager)
    : f_pager(pager)
    , f_workingDirectory("/") {
    f_threads.push_back(new Thread(entry_point, true, *this));
    f_next_fd = 0;
}

Process::Process(virtaddr_t entry_point)
    : f_pager(new Pager())
    , f_workingDirectory("/") {
    f_threads.push_back(new Thread(entry_point, true, *this));
    f_next_fd = 0;
}

Process* Process::construct_kernel_process(virtaddr_t entry_point) {
    return new Process(entry_point, &Pager::kernel());
}

void Process::die(u32_t exitCode) {
    Thread* thisThread = Thread::current();

    bool suicide = false;

    f_lock.lock();
    for(auto* thread : f_threads) {
        if(thread == thisThread) {
            suicide = true;
            continue;
        }

        auto current_state = thread->f_state;
        thread->f_state = DYING;

        if(current_state == RUNNING)
            send_task_switch_irq(thread->f_preferred_core);

        Scheduler::remove_thread(thread);

        if(!thread->f_watched) thread->schedule_finalization();
    }

    f_exitCode = exitCode;

    if(suicide) {
        if(!thisThread->f_watched)
            thisThread->schedule_finalization();
        else
            thisThread->f_state = DYING;

        // We should not return from an exit syscall.
        f_lock.unlock();
        while(true) force_task_switch();
    }
}

fd_t Process::add_stream(Stream* stream) {
    f_lock.lock();
    fd_t fd = f_next_fd++;
    f_streams.insert({ fd, stream });

    f_lock.unlock();
    return fd;
}

void Process::close_stream(fd_t fd) {
    f_lock.lock();

    f_streams.erase(fd);

    f_lock.unlock();
}

Stream* Process::get_stream(fd_t fd) {
    f_lock.lock();

    auto val = f_streams.at(fd);

    f_lock.unlock();

    if(!val) return 0;
    return *val;
}
