#pragma once

#include <types.h>

namespace kernel {
    struct Event {
        u64_t f_identified;

        size_t f_argc;
        u64_t* f_argv;

        bool f_consumed;
    };
}
