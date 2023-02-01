#pragma once

#include <types.h>
#include <defines.h>

#define UEVENT_NULL         0
#define UEVENT_LOAD_MODULE  1

#define UEVENT_ARGT_NULL    0
#define UEVENT_ARGT_STR     1

namespace kernel {
    struct UEventArg {
        u8_t f_type;
        u16_t f_size;
        u8_t reserved;

        char f_name[64];

        u8_t f_value[];
    } PACKED;

    struct UEvent {
        u64_t f_event_id;
        u32_t f_event_type;
        u32_t f_event_size;

        u8_t f_arg_count;
        u8_t f_reserved[3];

        UEventArg f_args[];
    } PACKED;
}

