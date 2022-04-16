#include <tasking/syscall.h>
#include <streams/filestream.hpp>
#include <fs/vfs.hpp>

using namespace kernel;

syscall_arg_t syscall_open(Process& proc, syscall_arg_t name, syscall_arg_t flags) {
    const char* path = (const char*)name;

    VNodePtr file;
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

syscall_arg_t syscall_close(Process& proc, syscall_arg_t fd) {
    proc.close_stream(fd);
    return 0;
}

syscall_arg_t syscall_read(Process& proc, syscall_arg_t fd, syscall_arg_t ptr, syscall_arg_t length) {
    auto* stream = proc.get_stream(fd);
    if(stream == 0) return -ERR_INVALID_FD;
    
    /// TODO: [16.04.2022] Check if ptr isn't outside of the program memory

    size_t size = stream->read((void*)ptr, length);
    return size;
}
