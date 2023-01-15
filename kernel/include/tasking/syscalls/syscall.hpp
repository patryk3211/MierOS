#pragma once

#include <tasking/syscall.h>

// ~Macro Magic~
#define DEF_SYSCALLN0(name) syscall_arg_t syscall_##name(Process& proc)
#define DEF_SYSCALLN1(name, a1) syscall_arg_t syscall_##name(Process& proc, syscall_arg_t a1)
#define DEF_SYSCALLN2(name, a1, a2) syscall_arg_t syscall_##name(Process& proc, syscall_arg_t a1, syscall_arg_t a2)
#define DEF_SYSCALLN3(name, a1, a2, a3) syscall_arg_t syscall_##name(Process& proc, syscall_arg_t a1, syscall_arg_t a2, syscall_arg_t a3)
#define DEF_SYSCALLN4(name, a1, a2, a3, a4) syscall_arg_t syscall_##name(Process& proc, syscall_arg_t a1, syscall_arg_t a2, syscall_arg_t a3, syscall_arg_t a4)
#define DEF_SYSCALLN5(name, a1, a2, a3, a4, a5) syscall_arg_t syscall_##name(Process& proc, syscall_arg_t a1, syscall_arg_t a2, syscall_arg_t a3, syscall_arg_t a4, syscall_arg_t a5)
#define DEF_SYSCALLN6(name, a1, a2, a3, a4, a5, a6) syscall_arg_t syscall_##name(Process& proc, syscall_arg_t a1, syscall_arg_t a2, syscall_arg_t a3, syscall_arg_t a4, syscall_arg_t a5, syscall_arg_t a6)

#define PASTE(a, b) a ## b
#define XPASTE(a, b) PASTE(a, b)

#define _ARG_8_MACRO(a1, a2, a3, a4, a5, a6, a7, a8, ...) a8
#define NARGS(...) _ARG_8_MACRO(dummy, ##__VA_ARGS__, 6, 5, 4, 3, 2, 1, 0)
#define SYSCALLN_(M, ...) M(__VA_ARGS__)

#define DEF_SYSCALL(name, ...) SYSCALLN_(XPASTE(DEF_SYSCALLN, NARGS(__VA_ARGS__)), name, ##__VA_ARGS__)
#define SYSCALL(index, name) syscall_table[(index)] = (syscall_func_t*)&syscall_##name
