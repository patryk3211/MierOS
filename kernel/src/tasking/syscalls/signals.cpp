#include <tasking/syscalls/syscall.hpp>
#include <asm/signal.h>

using namespace kernel;

struct SignalLock {
    Process& proc;

    SignalLock(Process& proc)
        : proc(proc) {
        proc.signal_lock();
    }

    ~SignalLock() {
        proc.signal_unlock();
    }
};

DEF_SYSCALL(sigmask, how, set, oldSet) {
    auto* thread = Thread::current();
    SignalLock lock(proc);

    if(oldSet) {
        VALIDATE_PTR(oldSet);
        *reinterpret_cast<sigset_t*>(oldSet) = thread->sigmask();
    }

    if(set) {
        VALIDATE_PTR(set);
        sigset_t bitSet = *reinterpret_cast<sigset_t*>(set);

        switch(how) {
            case SIG_BLOCK:
                thread->sigmask() |= bitSet;
                break;
            case SIG_UNBLOCK:
                thread->sigmask() &= ~bitSet;
                break;
            case SIG_SETMASK:
                thread->sigmask() = bitSet;
                break;
            default:
                return -EINVAL;
        }
    }

    return 0;
}

DEF_SYSCALL(sigaction, sigNum, action, oldAction) {
    if(sigNum >= 64)
        return -EINVAL;

    SignalLock lock(proc);
    SignalAction& kact = proc.action(sigNum);

    if(oldAction) {
        VALIDATE_PTR(oldAction);
        sigaction* act = reinterpret_cast<sigaction*>(oldAction);

        act->sa_mask = kact.mask;
        act->sa_flags = kact.flags;
        act->sa_restorer = kact.restorer;

        if(kact.flags & SA_SIGINFO) {
            act->sa_handler = 0;
            act->sa_sigaction = reinterpret_cast<void (*)(int, siginfo_t*, void*)>(kact.handler);
        } else {
            act->sa_handler = reinterpret_cast<void (*)(int)>(kact.handler);
            act->sa_sigaction = 0;
        }
    }

    if(action) {
        VALIDATE_PTR(action);
        sigaction* act = reinterpret_cast<sigaction*>(action);

        kact.mask = act->sa_mask;
        kact.flags = act->sa_flags;
        kact.restorer = act->sa_restorer;

        if(kact.flags & SA_SIGINFO) {
            VALIDATE_PTR(reinterpret_cast<uintptr_t>(act->sa_sigaction));
            kact.handler = reinterpret_cast<void (*)(...)>(act->sa_sigaction);
        } else {
            VALIDATE_PTR(reinterpret_cast<uintptr_t>(act->sa_handler));
            kact.handler = reinterpret_cast<void (*)(...)>(act->sa_handler);
        }
    }

    return 0;
}

DEF_SYSCALL(kill, pid, signal) {
    if(signal >= 64)
        return -EINVAL;

    auto* thread = Thread::get(pid);
    if(!thread || !thread->is_main())
        return -ESRCH;

    siginfo_t* info = new siginfo_t();
    info->si_pid = proc.pid();
    info->si_uid = proc.uid();
    info->si_signo = signal;

    thread->parent().signal(info);

    TRACE("(syscall) Process (pid = %d) signalled process (pid = %d) with signum = %d", proc.pid(), pid, signal);
    return 0;
}

DEF_SYSCALL(sigreturn) {
    TRACE("(syscall) Process (pid = %d) returning from signal", proc.pid());
    Thread::current()->load_signal_state();
    return 0;
}

