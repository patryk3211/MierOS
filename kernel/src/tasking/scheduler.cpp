#include <tasking/scheduler.hpp>
#include <sections.h>
#include <arch/cpu.h>

using namespace kernel;

ThreadQueue Scheduler::wait_queue = ThreadQueue();
ThreadQueue Scheduler::runnable_queue = ThreadQueue();
Scheduler* Scheduler::schedulers;
SpinLock Scheduler::queue_lock;

TEXT_FREE_AFTER_INIT Scheduler::Scheduler() : idle_stack(4096) {
    current_thread = 0;
    idle_ksp = (CPUState*)((virtaddr_t)idle_stack.ptr()+4096-sizeof(CPUState));

    idle_ksp->rip = (u64_t)&idle;
    idle_ksp->cs = 0x08;
    idle_ksp->rflags = 0x202;

    is_idle = false;
}

Scheduler::~Scheduler() {

}

CPUState* Scheduler::schedule(CPUState* current_state) {
    // Try to obtain a lock on the thread queue.
    if(!queue_lock.try_lock()) return current_state;

    if(current_thread == 0) {
        // We were in the idle task.
        idle_ksp = current_state;
    } else {
        current_thread->ksp = current_state;
        if(current_thread->state == RUNNING) {
            current_thread->state = RUNNABLE;
            runnable_queue.push_back(current_thread);
        } else {
            wait_queue.push_back(current_thread);
        }
        current_thread = 0;
    }

    Thread* new_thread = runnable_queue.get_optimal_thread(core_id);
    queue_lock.unlock();
    if(new_thread == 0) {
        // No thread needs to be executed.
        is_idle = true;
        return idle_ksp;
    } else {
        is_idle = false;
        new_thread->state = RUNNING;
        current_thread = new_thread;
        return current_thread->ksp;
    }
}

TEXT_FREE_AFTER_INIT void Scheduler::init(int core_count) {
    schedulers = new Scheduler[core_count];
    for(int i = 0; i < core_count; ++i) schedulers[i].core_id = i;
}

void Scheduler::idle() {
    int core_id = current_core();
    while(true) {
        asm volatile("hlt");
    }
}

Scheduler& Scheduler::scheduler(int core) {
    return schedulers[core];
}
