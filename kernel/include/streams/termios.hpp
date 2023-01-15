#pragma once

#include <types.h>

namespace kernel {
    // Termios mode flags
    typedef u32_t termios_flag_t;
    // Termios control characters
    typedef char termios_cc_t;

    #define CC_COUNT 16

    struct TermiosData {
        termios_flag_t f_input_flags;
        termios_flag_t f_output_flags;
        termios_flag_t f_control_flags;
        termios_flag_t f_local_flags;
        termios_cc_t f_control_characters[CC_COUNT];
    };
}