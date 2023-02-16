#include <arch/cpu.h>
#include <arch/interrupts.h>
#include <assert.h>
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

    f_next = 0;
    f_state = RUNNABLE;

    s_threads.insert({ f_pid, this });
}

void Thread::sleep(bool reschedule) {
    auto prevState = f_state;

    f_state = SLEEPING;
    Scheduler::remove_thread(this);
    if(reschedule) {
        if(Thread::current() == this) {
            force_task_switch();
        } else if(prevState == RUNNING) {
            send_task_switch_irq(f_preferred_core);
        }
    }
}

void Thread::wakeup() {
    ASSERT_F(f_state == SLEEPING, "Cannot wake up a thread that's not sleeping");
    f_state = RUNNABLE;
    
    Scheduler::schedule_thread(this);
}

Thread* Thread::current() {
    if(!Scheduler::is_initialized()) return 0;
    else return Scheduler::scheduler(current_core()).thread();
}

Thread* Thread::get(pid_t tid) {
    auto thread = s_threads.at(tid);
    return thread ? *thread : 0;
}

bool Thread::is_main() {
    return f_parent.main_thread() == this;
}

ThreadState Thread::get_state(bool sleep) {
    auto translateState = [](ThreadState inputState) {
        switch(inputState) {
            case RUNNING:
            case RUNNABLE:
            case SLEEPING:
                // This thread is nice and healthy. It is alive.
                return RUNNING;
            case DYING:
            case DEAD:
                // This thread is dead, it will be cleaned up before returning
                // from the wait pid syscall.
                return DEAD;
        }
    };

    auto state = translateState(f_state);
    if(state == RUNNING && sleep) {
        f_state_watchers.sleep();
        state = translateState(f_state);
    }

    return state;
}

void Thread::change_state(ThreadState newState) {
    f_state = newState;
    f_state_watchers.wakeup();
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

