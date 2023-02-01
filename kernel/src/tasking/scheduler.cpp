#include "locking/locker.hpp"
#include "tasking/thread.hpp"
#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <assert.h>
#include <defines.h>
#include <tasking/scheduler.hpp>

using namespace kernel;

NO_EXPORT ThreadQueue Scheduler::runnable_queue = ThreadQueue();
NO_EXPORT Scheduler* Scheduler::schedulers;
NO_EXPORT SpinLock Scheduler::queue_lock;

TEXT_FREE_AFTER_INIT Scheduler::Scheduler()
    : idle_stack(4096) {
    current_thread = 0;
    idle_ksp = (CPUState*)((virtaddr_t)idle_stack.ptr() + 4096 - sizeof(CPUState));

    idle_ksp->rip = (u64_t)&idle;
    idle_ksp->cs = 0x08;
    idle_ksp->rflags = 0x202;

    idle_ksp->cr3 = Pager::active().cr3();
    idle_ksp->rsp = (virtaddr_t)idle_stack.ptr() + 4096;
    idle_ksp->ss = 0x10;

    f_is_idle = false;
    f_first_switch = true;
}

Scheduler::~Scheduler() {
}

void Scheduler::pre_syscall(CPUState* current_state) {
    Thread::current()->f_syscall_state = current_state;
}

CPUState* Scheduler::post_syscall() {
    return Thread::current()->f_syscall_state;
}

CPUState* Scheduler::schedule(CPUState* current_state) {
    // Try to obtain a lock on the thread queue.
    if(!queue_lock.try_lock()) {
        current_state->next_switch_time = 1; // 1 us
        return current_state;
    }
    if(!f_first_switch) {
        if(current_thread == 0) {
            // We were in the idle task.
            idle_ksp = current_state;
        } else {
            current_thread->save_fpu_state();
            current_thread->f_ksp = current_state;
            task_switched();

            if(current_thread->f_state == RUNNING) {
                current_thread->f_state = RUNNABLE;
                runnable_queue.push_back(current_thread);
            }
            current_thread = 0;
        }
    } else
        f_first_switch = false;

    Thread* new_thread = runnable_queue.get_optimal_thread(core_id);
    queue_lock.unlock();
    if(new_thread == 0) {
        // No thread needs to be executed.
        f_is_idle = true;
        idle_ksp->next_switch_time = 250; // 250 us
        return idle_ksp;
    } else {
        f_is_idle = false;
        new_thread->f_state = RUNNING;
        current_thread = new_thread;
        current_thread->f_ksp->next_switch_time = 1000; // 1000 us -> 1 ms
        return current_thread->f_ksp;
    }
}

TEXT_FREE_AFTER_INIT void Scheduler::init(int core_count) {
    schedulers = new Scheduler[core_count];
    for(int i = 0; i < core_count; ++i) schedulers[i].core_id = i;

    register_task_switch_handler([](CPUState* state) -> CPUState* {
        return schedulers[current_core()].schedule(state);
    });
}

void Scheduler::idle() {
    //int core_id = current_core();
    while(true) {
        asm volatile("hlt");
    }
}

Scheduler& Scheduler::scheduler(int core) {
    return schedulers[core];
}

void Scheduler::schedule_process(Process& proc) {
    Locker lock(queue_lock);
    for(auto& thread : proc.f_threads) {
        if(thread->f_state != RUNNABLE)
            continue;
        runnable_queue.push_back(thread);
    }
}

void Scheduler::schedule_thread(Thread* thread) {
    ASSERT_F(thread->f_state == RUNNABLE, "Trying to run a thread that is not in the RUNNABLE state");
    Locker lock(queue_lock);
    runnable_queue.push_back(thread);
}

void Scheduler::remove_thread(Thread* thread) {
    Locker lock(queue_lock);
    runnable_queue.erase(thread);
}

bool Scheduler::is_initialized() {
    return schedulers != 0;
}
