#include <arch/interrupts.h>
#include <stdlib.h>
#include <tasking/syscall.h>

using namespace kernel;

typedef syscall_arg_t syscall_func_t(Process&, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t);

syscall_func_t* syscall_table[64];

extern syscall_arg_t syscall_exit(Process& proc, syscall_arg_t exitCode);

extern "C" void init_syscalls() {
    memset(syscall_table, 0, sizeof(syscall_table));

    syscall_table[1] = (syscall_func_t*)&syscall_exit;

    register_syscall_handler(&run_syscall);
}

extern "C" syscall_arg_t run_syscall(syscall_arg_t call, syscall_arg_t arg1, syscall_arg_t arg2, syscall_arg_t arg3, syscall_arg_t arg4, syscall_arg_t arg5, syscall_arg_t arg6) {
    if(syscall_table[call] != 0)
        return syscall_table[call](Thread::current()->parent(), arg1, arg2, arg3, arg4, arg5, arg6);
    else
        return -1;
}
