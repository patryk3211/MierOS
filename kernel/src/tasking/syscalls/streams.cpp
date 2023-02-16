#include "streams/directorystream.hpp"
#include <fs/vfs.hpp>
#include <streams/filestream.hpp>
#include <tasking/syscalls/syscall.hpp>
#include <asm/fcntl.h>

using namespace kernel;

DEF_SYSCALL(openat, name, flags, mode, dirfd) {
    const char* path = (const char*)name;

    VNodePtr root = nullptr;
    std::String<> resolvePath = path;
    if(path[0] != '/') {
        if((fd_t)dirfd == AT_FDCWD) {
            resolvePath = proc.cwd() + resolvePath;
        } else {
            auto* stream = proc.get_stream(dirfd);
            if(!stream || stream->type() != STREAM_TYPE_FILE)
                return -EBADF;

            root = static_cast<FileStream*>(stream)->node();
        }
    }

    bool followLink = !(flags & O_NOFOLLOW);
    auto ret = VFS::instance()->get_file(root, resolvePath.c_str(), { .resolve_link = followLink, .follow_links = true });
    if(!ret)
        return -ret.errno();

    if(!followLink && (*ret)->type() == VNode::LINK)
        return -ELOOP;

    Stream* stream = 0;
    if(flags & O_DIRECTORY) {
        if((*ret)->type() != VNode::DIRECTORY)
            return -ENOTDIR;

        auto* dstream = new DirectoryStream(*ret);
        dstream->flags() = flags;

        if(!(flags & O_PATH)) {
            auto result = dstream->open();
            if(!result) {
                delete dstream;
                return -result.errno();
            }
        }

        stream = dstream;
    } else {
        auto* fstream = new FileStream(*ret);
        fstream->flags() = flags;

        if(!(flags & O_PATH)) {
            auto result = fstream->open(mode);
            if(!result) {
                delete fstream;
                return -result.errno();
            }
        }

        stream = fstream;
    }

    fd_t fd = proc.add_stream(stream);

    TRACE("(syscall) Process (pid = %d) opened new fd = %d, file '%s'", proc.main_thread()->pid(), fd, path);
    return fd;
}

DEF_SYSCALL(close, fd) {
    proc.close_stream(fd);
    TRACE("(syscall) Process (pid = %d) closed fd = %d", proc.main_thread()->pid(), fd);
    return 0;
}

DEF_SYSCALL(read, fd, ptr, length) {
    auto* stream = proc.get_stream(fd);
    if(stream == 0) return -EBADF;

    /// TODO: [16.04.2022] Check if ptr isn't outside of the program memory

    auto ret = stream->read((void*)ptr, length);
    if(!ret)
        return -ret.errno();
    TRACE("(syscall) Process (pid = %d) read from fd %d to 0x%016x, requested length of %d, actual read size = %d",
            proc.main_thread()->pid(), fd, ptr, length, *ret);
    return *ret;
}

DEF_SYSCALL(write, fd, ptr, length) {
    auto* stream = proc.get_stream(fd);
    if(stream == 0) return -EBADF;

    /// TODO: [16.04.2022] Check if ptr isn't outside of the program memory

    auto ret = stream->write((const void*)ptr, length);
    if(!ret)
        return -ret.errno();
    TRACE("(syscall) Process (pid = %d) write to fd %d from 0x%016x, requested length of %d, actual write size = %d",
            proc.main_thread()->pid(), fd, ptr, length, *ret);
    return *ret;
}

DEF_SYSCALL(seek, fd, position, mode) {
    auto* stream = proc.get_stream(fd);
    if(stream == 0) return -EBADF;

    auto ret = stream->seek(position, mode);
    if(!ret)
        return -ret.errno();
    return *ret;
}

DEF_SYSCALL(ioctl, fd, request, arg) {
    auto* stream = proc.get_stream(fd);
    if(stream == 0) return -EBADF;

    if(stream->type() != STREAM_TYPE_FILE)
        return EPIPE;

    auto* fstream = (FileStream*)stream;
    auto result = fstream->ioctl(request, (void*)arg);

    syscall_arg_t resolvedResult = result ? *result : -result.errno();
    TRACE("(syscall) Process (pid = %d) ioctl request 0x%x on fd %d with arg = 0x%016x, result = %d",
            proc.main_thread()->pid(), request, fd, arg, resolvedResult);

    return resolvedResult;
}

DEF_SYSCALL(dup, oldfd, newfd, flags) {
    auto* stream = proc.get_stream(oldfd);
    if(stream == 0) return -EBADF;

    auto result = proc.add_stream(stream, newfd);
    return result;
}

DEF_SYSCALL(symlinkat, target, dirfd, linkpath) {
    VALIDATE_PTR(target);
    VALIDATE_PTR(linkpath);

    char* path = (char*)target;

    VNodePtr root = nullptr;
    std::String<> resolvePath = path;

    if(path[0] != '/') {
        if((fd_t)dirfd == AT_FDCWD) {
            resolvePath = proc.cwd() + resolvePath;
        } else {
            auto* stream = proc.get_stream(dirfd);
            if(!stream || stream->type() != STREAM_TYPE_FILE)
                return -EBADF;

            root = static_cast<FileStream*>(stream)->node();
        }
    }

    auto resolved = VFS::instance()->resolve_path(root, resolvePath.c_str(), { .resolve_link = true, .follow_links = true });
    if(!resolved)
        return -resolved.errno();

    auto result = resolved->key->filesystem()->symlink(resolved->key, resolved->value, (char*)linkpath);

    return result ? 0 : -result.errno();
}

