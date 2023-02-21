#pragma once

#include <types.h>
#include <defines.h>

namespace kernel {
    struct UEventArg {
        const char* name;
        const char* value;
    };

    struct UEvent {
        const char* buffer;

        const char* event_name;
        
        size_t argc;
        UEventArg argv[];

        ~UEvent();

        const char* get_arg(const char* name);
    };

    UEvent* parse_uevent(const void* data, size_t length);
}

