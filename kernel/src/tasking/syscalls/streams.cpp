#include <streams/directorystream.hpp>
#include <streams/pipestream.hpp>
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

    TRACE("(syscall) Process (pid = %d) opened new fd = %d, file '%s'", proc.pid(), fd, path);
    return fd;
}

DEF_SYSCALL(close, fd) {
    auto result = proc.close_stream(fd);
    TRACE("(syscall) Process (pid = %d) closed fd = %d", proc.main_thread()->pid(), fd);
    return result ? 0 : -result.errno();
}

DEF_SYSCALL(read, fd, ptr, length) {
    auto* stream = proc.get_stream(fd);
    if(stream == 0) return -EBADF;

    /// TODO: [16.04.2022] Check if ptr isn't outside of the program memory

    auto ret = stream->read((void*)ptr, length);
    if(!ret)
        return -ret.errno();
    TRACE("(syscall) Process (pid = %d) read from fd %d to 0x%016lx, requested length of %d, actual read size = %d",
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
    TRACE("(syscall) Process (pid = %d) write to fd %d from 0x%016lx, requested length of %d, actual write size = %d",
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
        return -EPIPE;

    auto* fstream = (FileStream*)stream;
    auto result = fstream->ioctl(request, (void*)arg);

    syscall_arg_t resolvedResult = result ? *result : -result.errno();
    TRACE("(syscall) Process (pid = %d) ioctl request 0x%x on fd %d with arg = 0x%016x, result = %d",
            proc.main_thread()->pid(), request, fd, arg, resolvedResult);

    return resolvedResult;
}

DEF_SYSCALL(dup, oldfd, newfd, flags) {
    auto result = proc.dup_stream(oldfd, newfd, flags);

    if(result)
        TRACE("(syscall) Process (pid = %d) duplicated fd %d onto fd %d", proc.pid(), oldfd, *result);
    return result ? *result : -result.errno();
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

DEF_SYSCALL(pipe, pipeStorage, flags) {
    VALIDATE_PTR(pipeStorage);

    auto result = PipeStream::make_pair();
    result.key->flags() = flags;
    result.value->flags() = flags;

    int* ptr = reinterpret_cast<int*>(pipeStorage);
    ptr[0] = proc.add_stream(result.key);
    ptr[1] = proc.add_stream(result.value);

    TRACE("(syscall) Process (pid = %d) opened new pipe pair (read = %d, write = %d)", proc.pid(), ptr[0], ptr[1]);
    return 0;
}

const int F_STATUS_FLAG_MASK = O_APPEND | O_ASYNC | O_DIRECT | O_NOATIME | O_NONBLOCK;
DEF_SYSCALL(fdflags, fd, flags) {
    auto wrapper = proc.get_stream_wrapper(fd);
    if(!wrapper)
        return wrapper.errno();

    switch(flags & FDF_OP_MASK) {
        case FDF_GETS:
            return wrapper->base().flags();
        case FDF_GETD: {
            auto flags = wrapper->flags();
            // Translate O_CLOEXEC into FD_CLOEXEC
            return (flags & O_CLOEXEC) ? FD_CLOEXEC : 0;
        }
        case FDF_SETS: {
            // First clear the modifiable flags in the stream
            wrapper->base().flags() &= ~F_STATUS_FLAG_MASK;
            // And then apply the modifiable flags onto the stream
            wrapper->base().flags() |= flags & F_STATUS_FLAG_MASK;
            // TODO: [28.02.2023] We might want to notify the underlying filesystem of the flag change.
        }
        case FDF_SETD: {
            // Set or clear the CLOEXEC flag
            if(flags & FD_CLOEXEC) {
                wrapper->flags() |= O_CLOEXEC;
            } else {
                wrapper->flags() &= ~O_CLOEXEC;
            }
        }
        default:
            return EINVAL;
    }
    return 0;
}

