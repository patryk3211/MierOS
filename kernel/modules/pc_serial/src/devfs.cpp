#include <errno.h>
#include <fs/devicefs.hpp>
#include "port.hpp"
#include <defines.h>
#include <fs/vnode.hpp>
#include <asm/ioctls.h>
#include <asm/fcntl.h>
#include <tasking/thread.hpp>
#include <tasking/process.hpp>

using namespace kernel;

extern u32_t working_ports;
extern Port* device_ports[];

#define CHECK_DEV(minor) \
    if(!((working_ports >> (minor)) & 1)) \
        return ENODEV;

ValueOrError<void> dev_open(u16_t minor, FileStream* stream, int mode) {
    UNUSED(stream);
    UNUSED(mode);

    CHECK_DEV(minor);

    ++device_ports[minor]->openCount;

    if(!(stream->flags() & O_NOCTTY)) {
        auto& proc = Thread::current()->parent();
        auto& cttyRef = proc.ctty();
        if(!cttyRef) {
            // Make this the controlling terminal of the process
            cttyRef = stream->node();
            device_ports[minor]->termiosHelper.process_group() = proc.pgid();
        }
    }
    return { };
}

ValueOrError<void> dev_close(u16_t minor, FileStream* stream) {
    UNUSED(stream);

    CHECK_DEV(minor);

    --device_ports[minor]->openCount;
    return { };
}

ValueOrError<size_t> dev_read(u16_t minor, FileStream* stream, void* buffer, size_t length) {
    UNUSED(stream);

    CHECK_DEV(minor);

    return device_ports[minor]->termiosHelper.stream_read(buffer, length);
}

ValueOrError<size_t> dev_write(u16_t minor, FileStream* stream, const void* buffer, size_t length) {
    UNUSED(stream);

    CHECK_DEV(minor);

    return device_ports[minor]->termiosHelper.stream_write(buffer, length);
}

ValueOrError<int> dev_ioctl(u16_t minor, u64_t request, void* arg) {
    CHECK_DEV(minor);

    switch(request) {
        case TCGETS:
            if(arg)
                memcpy(arg, device_ports[minor]->termiosHelper.termios(), sizeof(termios));
            return 0;
        case TCSETS:
            device_ports[minor]->set_termios(arg, SET_IMMEDIATE);
            return 0;
        case TCSETSW:
            device_ports[minor]->set_termios(arg, SET_DRAIN);
            return 0;
        case TCSETSF:
            device_ports[minor]->set_termios(arg, SET_DRAIN | SET_FLUSH);
            return 0;
        case TIOCGPGRP:
            *static_cast<pid_t*>(arg) = device_ports[minor]->termiosHelper.process_group();
            return 0;
        case TIOCSPGRP:
            device_ports[minor]->termiosHelper.process_group() = *static_cast<pid_t*>(arg);
            return 0;
        default:
            return EINVAL;
    }
}

DeviceFunctionTable device_function_table = {
    .open = &dev_open,
    .close = &dev_close,

    .read = &dev_read,
    .write = &dev_write,

    .ioctl = &dev_ioctl
};

