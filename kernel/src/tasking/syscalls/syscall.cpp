#include <arch/interrupts.h>
#include <stdlib.h>
#include <tasking/syscalls/syscall.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"

using namespace kernel;

typedef syscall_arg_t syscall_func_t(Process&, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t);

syscall_func_t* syscall_table[64];

// Syscall definitions
DEF_SYSCALL(exit, exitCode);
DEF_SYSCALL(open, name, flags);
DEF_SYSCALL(close, fd);
DEF_SYSCALL(read, fd, ptr, length);
DEF_SYSCALL(write, fd, ptr, length);
DEF_SYSCALL(fork);
DEF_SYSCALL(seek, fd, position, mode);
DEF_SYSCALL(mmap, ptr, length, prot, flags, fd, offset);
DEF_SYSCALL(munmap, ptr, length);
DEF_SYSCALL(execve, filename, argv, envp);
DEF_SYSCALL(arch_prctl, func, ptr);
DEF_SYSCALL(init_module, modPtr, argv);
DEF_SYSCALL(uevent_poll, eventPtr, flags);
DEF_SYSCALL(uevent_complete, eventPtr, status);

extern "C" void init_syscalls() {
    memset(syscall_table, 0, sizeof(syscall_table));

    SYSCALL(1,  exit);
    SYSCALL(2,  open);
    SYSCALL(3,  close);
    SYSCALL(4,  read);
    SYSCALL(5,  write);
    SYSCALL(6,  fork);
    SYSCALL(7,  seek);
    SYSCALL(8,  mmap);
    SYSCALL(9,  munmap);
    SYSCALL(10, execve);
    SYSCALL(11, arch_prctl);
    SYSCALL(12, init_module);
    SYSCALL(13, uevent_poll);
    SYSCALL(14, uevent_complete);

    register_syscall_handler(&run_syscall);
}

extern "C" syscall_arg_t run_syscall(syscall_arg_t call, syscall_arg_t arg1, syscall_arg_t arg2, syscall_arg_t arg3, syscall_arg_t arg4, syscall_arg_t arg5, syscall_arg_t arg6) {
    if(syscall_table[call] != 0)
        return syscall_table[call](Thread::current()->parent(), arg1, arg2, arg3, arg4, arg5, arg6);
    else
        return -1;
}

#pragma GCC diagnostic pop
