#include <tasking/syscall.h>
#include <tasking/syscalls/map.hpp>
#include <errno.h>

using namespace kernel;

syscall_arg_t syscall_mmap(Process& proc, syscall_arg_t ptr, syscall_arg_t length, syscall_arg_t prot, syscall_arg_t flags, syscall_arg_t fd, syscall_arg_t offset) {
    virtaddr_t addr = 0;
    size_t page_len = (length >> 12) + ((length & 0xFFF) == 0 ? 0 : 1);
    if(ptr != 0) {
        /// TODO: Validate the ptr and select the address based on it.
        return -ERR_UNIMPLEMENTED;
    } else {
        proc.pager().lock();
        addr = proc.pager().getFreeRange(MMAP_MIN_ADDR, page_len);
        proc.pager().unlock();
        if(addr == 0 || addr >= KERNEL_START) return -ERR_NO_MEMORY;
    }

    if(flags & MMAP_FLAG_ANONYMOUS) {
        // No file backing this mapping
        proc.alloc_pages(addr, page_len, flags, prot);
    }

    return addr;
}
