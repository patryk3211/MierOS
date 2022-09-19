#ifndef _MIEROS_KERNEL_TASKING_SYSCALL_H
#define _MIEROS_KERNEL_TASKING_SYSCALL_H

#include <tasking/process.hpp>
#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef u64_t syscall_arg_t;

void init_syscalls();
syscall_arg_t run_syscall(syscall_arg_t call, syscall_arg_t arg1, syscall_arg_t arg2, syscall_arg_t arg3, syscall_arg_t arg4, syscall_arg_t arg5, syscall_arg_t arg6);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_TASKING_SYSCALL_H
