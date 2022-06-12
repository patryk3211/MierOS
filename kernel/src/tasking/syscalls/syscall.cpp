#include <arch/interrupts.h>
#include <stdlib.h>
#include <tasking/syscall.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"

using namespace kernel;

typedef syscall_arg_t syscall_func_t(Process&, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t, syscall_arg_t);

syscall_func_t* syscall_table[64];

extern syscall_arg_t syscall_exit(Process& proc, syscall_arg_t exitCode);
extern syscall_arg_t syscall_open(Process& proc, syscall_arg_t name, syscall_arg_t flags);
extern syscall_arg_t syscall_close(Process& proc, syscall_arg_t fd);
extern syscall_arg_t syscall_read(Process& proc, syscall_arg_t fd, syscall_arg_t ptr, syscall_arg_t length);
extern syscall_arg_t syscall_write(Process& proc, syscall_arg_t fd, syscall_arg_t ptr, syscall_arg_t length);
extern syscall_arg_t syscall_fork(Process& proc);
extern syscall_arg_t syscall_seek(Process& proc, syscall_arg_t fd, syscall_arg_t pos, syscall_arg_t mode);

extern "C" void init_syscalls() {
    memset(syscall_table, 0, sizeof(syscall_table));

    syscall_table[1] = (syscall_func_t*)&syscall_exit;
    syscall_table[2] = (syscall_func_t*)&syscall_open;
    syscall_table[3] = (syscall_func_t*)&syscall_close;
    syscall_table[4] = (syscall_func_t*)&syscall_read;
    syscall_table[5] = (syscall_func_t*)&syscall_write;
    syscall_table[6] = (syscall_func_t*)&syscall_fork;
    syscall_table[7] = (syscall_func_t*)&syscall_seek;

    register_syscall_handler(&run_syscall);
}

extern "C" syscall_arg_t run_syscall(syscall_arg_t call, syscall_arg_t arg1, syscall_arg_t arg2, syscall_arg_t arg3, syscall_arg_t arg4, syscall_arg_t arg5, syscall_arg_t arg6) {
    if(syscall_table[call] != 0)
        return syscall_table[call](Thread::current()->parent(), arg1, arg2, arg3, arg4, arg5, arg6);
    else
        return -1;
}

#pragma GCC diagnostic pop
