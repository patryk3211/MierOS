#include <tasking/syscalls/syscall.hpp>
#include <tasking/syscalls/map.hpp>
#include <streams/filestream.hpp>
#include <errno.h>

using namespace kernel;

DEF_SYSCALL(mmap, ptr, length, prot, flags, fd, offset) {
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
        return addr;
    }

    auto* stream = proc.get_stream(fd);
    if(stream->type() != STREAM_TYPE_FILE) {
        // The file descriptor has to refer to a file
        return -ERR_INVALID;
    }

    if((offset & 0xFFF) != 0) {
        // Offset has to be page aligned
        return -ERR_INVALID;
    }

    FileStream* fstream = (FileStream*)stream;
    FilePage* page = new FilePage(fstream->node(), addr, offset, (flags & MMAP_FLAG_SHARED), (prot & MMAP_PROT_WRITE), (prot & MMAP_PROT_EXEC));

    proc.file_pages(addr, page_len, page);

    return addr;
}

DEF_SYSCALL(munmap, ptr, length) {
    size_t page_len = (length >> 12) + ((length & 0xFFF) == 0 ? 0 : 1);

    // Validate the pointer
    ptr = ptr & ~0xFFF;
    if(ptr < MMAP_MIN_ADDR || ptr >= KERNEL_START || ptr + (page_len << 12) > KERNEL_START || ptr + (page_len << 12) < ptr) return -ERR_INVALID;

    proc.unmap_pages(ptr, page_len);
    return 0;
}
