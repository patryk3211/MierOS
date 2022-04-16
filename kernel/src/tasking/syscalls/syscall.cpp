#include <tasking/syscall.h>
#include <stdlib.h>

using namespace kernel;

typedef syscall_arg_t syscall_func_t(Process&, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t);

syscall_func_t* syscall_table[64];

extern syscall_arg_t syscall_exit(Process& proc, syscall_arg_t exitCode);

extern "C" void init_syscall_table() {
    memset(syscall_table, 0, sizeof(syscall_table));

    syscall_table[1] = (syscall_func_t*)&syscall_exit;
}

extern "C" syscall_arg_t run_syscall(syscall_arg_t arg1, syscall_arg_t arg2, syscall_arg_t arg3, syscall_arg_t arg4, syscall_arg_t arg5, syscall_arg_t arg6) {
    if(syscall_table[arg1] != 0)
        return syscall_table[arg1](Thread::current()->parent(), arg2, arg3, arg4, arg5, arg6);
    else
        return -1;
}
