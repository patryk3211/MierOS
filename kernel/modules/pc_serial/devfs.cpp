#include <errno.h>
#include <fs/devicefs.hpp>
#include "port.hpp"
#include <defines.h>
#include <fs/vnode.hpp>
#include <asm/ioctls.h>

using namespace kernel;

static u32_t working_ports;
extern Port* device_ports[];

ValueOrError<void> dev_open(u16_t minor, FileStream* stream, int mode) {
    UNUSED(stream);
    UNUSED(mode);

    if(!((working_ports >> minor) & 1))
        return ENODEV;

    ++device_ports[minor]->openCount;
    return { };
}

ValueOrError<void> dev_close(u16_t minor, FileStream* stream) {
    UNUSED(stream);

    if(!((working_ports >> minor) & 1))
        return ENODEV;

    --device_ports[minor]->openCount;
    return { };
}

ValueOrError<size_t> dev_read(u16_t minor, FileStream* stream, void* buffer, size_t length) {
    UNUSED(stream);

    if(!((working_ports >> minor) & 1))
        return ENODEV;

    return device_ports[minor]->termiosHelper.stream_read(buffer, length);
}

ValueOrError<size_t> dev_write(u16_t minor, FileStream* stream, const void* buffer, size_t length) {
    UNUSED(stream);

    if(!((working_ports >> minor) & 1))
        return ENODEV;

    return device_ports[minor]->termiosHelper.stream_write(buffer, length);
}

ValueOrError<int> dev_ioctl(u16_t minor, u64_t request, void* arg) {
    if(!((working_ports >> minor) & 1))
        return ENODEV;

    switch(request) {
        case TCGETS:
            if(arg)
                memcpy(arg, device_ports[minor]->termiosHelper.termios(), sizeof(termios));
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

