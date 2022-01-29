#include <tasking/thread.hpp>
#include <tasking/scheduler.hpp>
#include <arch/cpu.h>
#include <tasking/process.hpp>
#include <arch/interrupts.h>
#include <locking/locker.hpp>

using namespace kernel;

#define KERNEL_STACK_SIZE 1024*16 // 16 KiB stack

NO_EXPORT pid_t Thread::next_pid = 1;
NO_EXPORT std::UnorderedMap<pid_t, Thread*> Thread::threads;
NO_EXPORT SpinLock Thread::pid_lock;

pid_t Thread::generate_pid() {
    Locker locker(pid_lock);
    while(true) {
        if(threads.find(next_pid) == threads.end()) return next_pid++;
        ++next_pid;
    }
}

Thread::Thread(u64_t ip, bool isKernel, Process& process) : kernel_stack(KERNEL_STACK_SIZE), parent(process) {
    preferred_core = -1;
    ksp = (CPUState*)((virtaddr_t)kernel_stack.ptr()+KERNEL_STACK_SIZE-sizeof(CPUState));

    memset(ksp, 0, sizeof(CPUState));
    ksp->cr3 = process.pager().cr3();
    ksp->rip = ip;
    ksp->rflags = 0x202;

    ksp->cs = isKernel ? 0x08 : 0x1B;
    ksp->ss = isKernel ? 0x10 : 0x23;

    if(isKernel) ksp->rsp = (virtaddr_t)kernel_stack.ptr()+KERNEL_STACK_SIZE;

    next = 0;
    state = RUNNABLE;
}

void Thread::sleep(std::Function<bool()>& until) {
    blockers.push_back(until);
    state = SLEEPING;
    if(Thread::current() == this) {
        force_task_switch();
    }
}

bool Thread::try_wakeup() {
    auto iter = blockers.begin();
    while(iter != blockers.end()) {
        if((*iter)()) {
            iter = blockers.erase(iter);
        } else ++iter;
    }
    if(blockers.empty()) {
        state = RUNNABLE;
        return true;
    } else return false;
}

Thread* Thread::current() {
    return Scheduler::scheduler(current_core()).thread();
}
