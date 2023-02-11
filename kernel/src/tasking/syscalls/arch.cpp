#include <tasking/syscalls/syscall.hpp>
#include <tasking/syscalls/arch.hpp>
#include <arch/cpu.h>

using namespace kernel;

DEF_SYSCALL(arch_prctl, func, ptr) {
    UNUSED(proc);

    switch(func) {
        case ARCH_SET_FS:
            Thread::current()->set_fs(*((u64_t*)ptr));
            TRACE("(syscall) Thread (tid = %d) set new fs = 0x%x16\n", Thread::current()->pid(), ptr);
            return 0;
        case ARCH_GET_FS:
            *((u64_t*)ptr) = Thread::current()->get_fs();
            return 0;
        default:
            return -EINVAL;
    }
}
