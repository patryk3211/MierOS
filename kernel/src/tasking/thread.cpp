#include <tasking/thread.hpp>

using namespace kernel;

#define KERNEL_STACK_SIZE 1024*16 // 16 KiB stack

Thread::Thread(u64_t ip, bool isKernel, Pager& pager) : kernel_stack(KERNEL_STACK_SIZE) {
    preferred_core = -1;
    ksp = (CPUState*)((virtaddr_t)kernel_stack.ptr()+KERNEL_STACK_SIZE-sizeof(CPUState));

    memset(ksp, 0, sizeof(CPUState));
    ksp->cr3 = pager.cr3();
    ksp->rip = ip;
    ksp->rflags = 0x202;

    next = 0;
}

void Thread::sleep(std::Function<bool()>& until) {
    blockers.push_back(until);
    state = SLEEPING;
    if(Thread::current() == this) {
        /// TODO: [22.01.2022] Force the scheduler to reschedule.
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
    return 0;
}
