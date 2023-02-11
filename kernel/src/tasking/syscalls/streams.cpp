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

    TRACE("(syscall) Process (pid = %d) opened new fd = %d, file '%s'\n", proc.main_thread()->pid(), fd, path);
    return fd;
}

DEF_SYSCALL(close, fd) {
    proc.close_stream(fd);
    TRACE("(syscall) Process (pid = %d) closed fd = %d\n", proc.main_thread()->pid(), fd);
    return 0;
}

DEF_SYSCALL(read, fd, ptr, length) {
    auto* stream = proc.get_stream(fd);
    if(stream == 0) return -EINVAL;

    /// TODO: [16.04.2022] Check if ptr isn't outside of the program memory

    size_t size = stream->read((void*)ptr, length);
    TRACE("(syscall) Process (pid = %d) read from fd %d to 0x%x16, requested length of %d, actual read size = %d\n",
            proc.main_thread()->pid(), fd, ptr, length, size);
    return size;
}

DEF_SYSCALL(write, fd, ptr, length) {
    auto* stream = proc.get_stream(fd);
    if(stream == 0) return -EINVAL;

    /// TODO: [16.04.2022] Check if ptr isn't outside of the program memory

    size_t size = stream->write((const void*)ptr, length);
    TRACE("(syscall) Process (pid = %d) write to fd %d from 0x%x16, requested length of %d, actual write size = %d\n",
            proc.main_thread()->pid(), fd, ptr, length, size);
    return size;
}

DEF_SYSCALL(seek, fd, position, mode) {
    auto* stream = proc.get_stream(fd);
    if(stream == 0) return -EINVAL;

    return stream->seek(position, mode);
}

DEF_SYSCALL(ioctl, fd, request, arg) {
    auto* stream = proc.get_stream(fd);
    if(stream == 0) return -EINVAL;

    if(stream->type() != STREAM_TYPE_FILE)
        return EPIPE;
}


