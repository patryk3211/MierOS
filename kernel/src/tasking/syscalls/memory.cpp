#include <tasking/syscall.h>
#include <tasking/syscalls/map.hpp>
#include <errno.h>

using namespace kernel;

syscall_arg_t syscall_mmap(Process& proc, syscall_arg_t ptr, syscall_arg_t length, syscall_arg_t prot, syscall_arg_t flags, syscall_arg_t fd, syscall_arg_t offset) {
    size_t page_len = (length >> 12) + ((length & 0xFFF) == 0 ? 0 : 1);

    /// TODO: This can only happen if FIXED flag is not specified, otherwise if the address is not aligned, this syscall should fail
    ptr = ptr & ~0xFFF;
    if(ptr != 0 && (ptr < MMAP_MIN_ADDR || ptr >= KERNEL_START ||
        ptr + (page_len << 12) > KERNEL_START || ptr + (page_len << 12) < ptr)) return -ERR_INVALID;

    virtaddr_t addr = proc.get_free_addr(ptr, page_len);

    if(prot == 0) {
        proc.null_pages(addr, page_len);
        return addr;
    }

    if(flags & MMAP_FLAG_ANONYMOUS) {
        // No file backing this mapping
        proc.alloc_pages(addr, page_len, flags, prot);
    }

    return addr;
}
