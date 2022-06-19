#include <tasking/syscall.h>
#include <tasking/syscalls/map.hpp>

using namespace kernel;

syscall_arg_t syscall_mmap(Process& proc, syscall_arg_t ptr, syscall_arg_t length, syscall_arg_t prot, syscall_arg_t flags, syscall_arg_t fd, syscall_arg_t offset) {
    
}
