#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <locking/locker.hpp>
#include <tasking/process.hpp>
#include <tasking/scheduler.hpp>
#include <tasking/thread.hpp>

using namespace kernel;

#define KERNEL_STACK_SIZE 1024 * 16 // 16 KiB stack

NO_EXPORT pid_t Thread::s_next_pid = 1;
NO_EXPORT std::UnorderedMap<pid_t, Thread*> Thread::s_threads;
NO_EXPORT std::List<pid_t> Thread::s_finalizable_threads;
NO_EXPORT SpinLock Thread::s_pid_lock;

pid_t Thread::generate_pid() {
    Locker locker(s_pid_lock);
    while(true) {
        if(s_threads.find(s_next_pid) == s_threads.end()) return s_next_pid++;
        ++s_next_pid;
    }
}

Thread::Thread(u64_t ip, bool isKernel, Process& process)
    : f_kernel_stack(KERNEL_STACK_SIZE)
    , f_fpu_state(512)
    , f_parent(process)
    , f_pid(generate_pid()) {
    f_preferred_core = -1;
    f_ksp = (CPUState*)((virtaddr_t)f_kernel_stack.ptr() + KERNEL_STACK_SIZE - sizeof(CPUState));

    memset(f_ksp, 0, sizeof(CPUState));
    f_ksp->cr3 = process.pager().cr3();
    f_ksp->rip = ip;
    f_ksp->rflags = 0x202;

    f_ksp->cs = isKernel ? 0x08 : 0x1B;
    f_ksp->ss = isKernel ? 0x10 : 0x23;

    if(isKernel) f_ksp->rsp = (virtaddr_t)f_kernel_stack.ptr() + KERNEL_STACK_SIZE;

    f_current_module = 0;

    f_watched = false;

    f_next = 0;
    f_state = RUNNABLE;
}

void Thread::sleep(std::Function<bool()>& until) {
    f_blockers.push_back(until);
    f_state = SLEEPING;
    if(Thread::current() == this) {
        force_task_switch();
    }
}

bool Thread::try_wakeup() {
    auto iter = f_blockers.begin();
    while(iter != f_blockers.end()) {
        if((*iter)()) {
            iter = f_blockers.erase(iter);
        } else
            ++iter;
    }
    if(f_blockers.empty()) {
        f_state = RUNNABLE;
        return true;
    } else
        return false;
}

Thread* Thread::current() {
    if(!Scheduler::is_initialized()) return 0;
    else return Scheduler::scheduler(current_core()).thread();
}

void Thread::schedule_finalization() {
    s_finalizable_threads.push_back(f_pid);
    f_state = DEAD;
    // At this point, if this thread is the current thread, it can get safely put out of the scheduler list.
}

void Thread::make_ks(virtaddr_t ip, virtaddr_t sp) {
    f_syscall_state = (CPUState*)((virtaddr_t)f_kernel_stack.ptr() + KERNEL_STACK_SIZE - sizeof(CPUState));

    memset(f_syscall_state, 0, sizeof(CPUState));

    // Some of this stuff is architecture specific
    // and should be moved to the appropriate place
    f_syscall_state->cr3 = f_parent.pager().cr3();
    f_syscall_state->rip = ip;
    f_syscall_state->rflags = 0x202;
    f_syscall_state->rsp = sp;

    f_syscall_state->cs = 0x1B;
    f_syscall_state->ss = 0x23;
}

void Thread::set_fs(virtaddr_t fs_base) {
    f_syscall_state->fs = fs_base;
}

virtaddr_t Thread::get_fs() {
    return f_syscall_state->fs;
}

void Thread::save_fpu_state() {
    auto* memory = f_fpu_state.ptr<uint8_t>();
    // We only save the state if the FPU is enabled
    asm volatile(
        "mov %%cr0, %%rax\n"
        "test $0x08, %%rax\n"
        "jnz 0f\n"
        "fxsave64 %0\n"
        "0: nop"
        :
        : "m"(*memory)
        : "rax");
}

void Thread::load_fpu_state() {
    auto* memory = f_fpu_state.ptr<uint8_t>();
    asm volatile("fxrstor64 %0" :: "m"(*memory));
}
