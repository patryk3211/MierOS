#include <tasking/syscalls/syscall.hpp>
#include <tasking/syscalls/map.hpp>
#include <streams/filestream.hpp>
#include <errno.h>

#include <util/profile.hpp>

using namespace kernel;

DEF_SYSCALL(mmap, ptr, length, prot, flags, fd, offset) {
    size_t page_len = (length >> 12) + ((length & 0xFFF) == 0 ? 0 : 1);

    /// TODO: This can only happen if FIXED flag is not specified, otherwise if the address is not aligned, this syscall should fail
    ptr = ptr & ~0xFFF;
    if(ptr != 0 && (ptr < MMAP_MIN_ADDR || ptr >= KERNEL_START ||
        ptr + (page_len << 12) > KERNEL_START || ptr + (page_len << 12) < ptr)) return -EINVAL;

    virtaddr_t addr = proc.get_free_addr(ptr, page_len);

    if(prot == 0) {
        proc.null_pages(addr, page_len);
        TRACE("(syscall) Process (pid = %d) mapped %d bytes starting at 0x%016lx as NULL", proc.pid(), page_len << 12, addr);
        return addr;
    }

    if(flags & MMAP_FLAG_ANONYMOUS) {
        // No file backing this mapping
        proc.alloc_pages(addr, page_len, flags, prot);
        TRACE("(syscall) Process (pid = %d) mapped %d bytes starting at 0x%016lx as ANON", proc.pid(), page_len << 12, addr);
        return addr;
    }

    auto* stream = proc.get_stream(fd);
    if(stream->type() != STREAM_TYPE_FILE) {
        // The file descriptor has to refer to a file
        return -EBADF;
    }

    if((offset & 0xFFF) != 0) {
        // Offset has to be page aligned
        return -EINVAL;
    }

    FileStream* fstream = (FileStream*)stream;

    proc.file_pages(addr, page_len, fstream->node(), offset, (flags & MMAP_FLAG_SHARED), (prot & MMAP_PROT_WRITE), (prot & MMAP_PROT_EXEC));
    TRACE("(syscall) Process (pid = %d) mapped %d bytes starting at 0x%016lx to FILE with offset %d", proc.pid(), page_len << 12, addr, offset);

    return addr;
}

DEF_SYSCALL(munmap, ptr, length) {
    size_t page_len = (length >> 12) + ((length & 0xFFF) == 0 ? 0 : 1);

    // Validate the pointer
    ptr = ptr & ~0xFFF;
    if(ptr < MMAP_MIN_ADDR || ptr >= KERNEL_START || ptr + (page_len << 12) > KERNEL_START || ptr + (page_len << 12) < ptr) return -EINVAL;

    proc.unmap_pages(ptr, page_len);
    TRACE("(syscall) Process (pid = %d) unmapped %d bytes starting at 0x%016lx", proc.pid(), page_len << 12, ptr);
    return 0;
}
