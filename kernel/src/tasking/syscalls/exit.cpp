#include <tasking/syscall.h>

using namespace kernel;

syscall_arg_t syscall_exit(Process& proc, syscall_arg_t exitCode) {
    proc.die(exitCode);
    return 0;
}
