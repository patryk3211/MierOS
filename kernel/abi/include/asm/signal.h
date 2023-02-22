#ifndef _ASM_SIGNAL_H
#define _ASM_SIGNAL_H

#if !defined(__KERNEL__)
#include <abi-bits/pid_t.h>
#include <abi-bits/uid_t.h>
#include <bits/size_t.h>
#else
#include <types.h>
#endif

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

// constants for sigprocmask()
#define SIG_BLOCK 1
#define SIG_UNBLOCK 2
#define SIG_SETMASK 3

typedef unsigned long sigset_t;

union sigval {
	int sival_int;
	void *sival_ptr;
};

typedef struct {
	int si_signo;
	int si_code;
	int si_errno;
	pid_t si_pid;
	uid_t si_uid;
	void *si_addr;
	int si_status;
	union sigval si_value;
} siginfo_t;

#define SIG_DFL ((__sighandler)(void *)( 0))
#define SIG_IGN ((__sighandler)(void *)(-1))
#define SIG_ERR ((__sighandler)(void *)(-2))

#define SA_NOCLDSTOP (1 << 0)
#define SA_ONSTACK (1 << 1)
#define SA_RESETHAND (1 << 2)
#define SA_RESTART (1 << 3)
#define SA_SIGINFO (1 << 4)
#define SA_NOCLDWAIT (1 << 5)
#define SA_NODEFER (1 << 6)

struct sigaction {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t*, void*);
    sigset_t sa_mask;
    int sa_flags;
};

#if defined(__x86_64__)
typedef struct {
    sigset_t oldmask;

    uint64_t gregs[16];
    uint64_t ip, flags;

    uint8_t fpu_state[512];
} ksigcontext_t;
#endif

#if defined(__cplusplus)
}
#endif

#endif

