#include <fs/vfs.hpp>
#include <streams/filestream.hpp>
#include <tasking/syscalls/syscall.hpp>

using namespace kernel;

DEF_SYSCALL(open, name, flags) {
    const char* path = (const char*)name;

    VNodePtr file = nullptr;
    if(path[0] == '/') {
        auto ret = VFS::instance()->get_file(nullptr, path, {});
        if(!ret) return -ret.errno();
        file = *ret;
    } else {
        auto ret = VFS::instance()->get_file(nullptr, (proc.cwd() + path).c_str(), {});
        if(!ret) return -ret.errno();
        file = *ret;
    }

    auto* fstream = new FileStream(file);

    auto ret = fstream->open(0);
    if(!ret) {
        delete fstream;
        return -ret.errno();
    }

    fd_t fd = proc.add_stream(fstream);

    return fd;
}

DEF_SYSCALL(close, fd) {
    proc.close_stream(fd);
    return 0;
}

DEF_SYSCALL(read, fd, ptr, length) {
    auto* stream = proc.get_stream(fd);
    if(stream == 0) return -ERR_INVALID;

    /// TODO: [16.04.2022] Check if ptr isn't outside of the program memory

    size_t size = stream->read((void*)ptr, length);
    return size;
}

DEF_SYSCALL(write, fd, ptr, length) {
    auto* stream = proc.get_stream(fd);
    if(stream == 0) return -ERR_INVALID;

    /// TODO: [16.04.2022] Check if ptr isn't outside of the program memory

    size_t size = stream->write((const void*)ptr, length);
    return size;
}

DEF_SYSCALL(seek, fd, position, mode) {
    auto* stream = proc.get_stream(fd);
    if(stream == 0) return -ERR_INVALID;

    return stream->seek(position, mode);
}
