#include <arch/interrupts.h>
#include <tasking/process.hpp>
#include <tasking/scheduler.hpp>

using namespace kernel;

Process::Process(virtaddr_t entry_point, Pager* pager)
    : f_pager(pager) {
    f_threads.push_back(new Thread(entry_point, true, *this));
}

Process::Process(virtaddr_t entry_point)
    : f_pager(new Pager()) {
    f_threads.push_back(new Thread(entry_point, true, *this));
}

Process* Process::construct_kernel_process(virtaddr_t entry_point) {
    return new Process(entry_point, &Pager::kernel());
}

void Process::die(u32_t exitCode) {
    Thread* thisThread = Thread::current();

    bool suicide = false;

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
        while(true) force_task_switch();
    }
}
