#pragma once

#include <types.h>

// Input flags
#define TERM_IGNBRK     1
#define TERM_BRKINT     2
#define TERM_IGNPAR     4
#define TERM_PARMRK     8
#define TERM_INPCK      16
#define TERM_ISTRIP     32
#define TERM_INLCR      64
#define TERM_IGNCR      128
#define TERM_ICRNL      256
#define TERM_IXON       512
#define TERM_IXANY      1024
#define TERM_IXOFF      2048

// Output flags
#define TERM_OPOST      1
#define TERM_ONLCR      2
#define TERM_OCRNL      4
#define TERM_ONOCR      8
#define TERM_ONLRET     16
#define TERM_OFILL      32
#define TERM_OFDEL      64

// Control flags
#define TERM_CS5        0
#define TERM_CS6        1
#define TERM_CS7        2
#define TERM_CS8        3
#define TERM_CSIZE      3
#define TERM_CSTOPB     4
#define TERM_CREAD      8
#define TERM_PARENB     16
#define TERM_PARODD     32
#define TERM_HUPCL      64
#define TERM_CLOCAL     128

// Local flags
#define TERM_ISIG       1
#define TERM_ICANON     2
#define TERM_ECHO       4
#define TERM_ECHOE      8
#define TERM_ECHOK      16
#define TERM_ECHONL     32
#define TERM_NOFLSH     64
#define TERM_TOSTOP     128

// Control characters
#define TERM_VEOF       0
#define TERM_VEOL       1
#define TERM_VERASE     2
#define TERM_VINTR      3
#define TERM_VKILL      4
#define TERM_VMIN       5
#define TERM_VQUIT      6
#define TERM_VSTART     7
#define TERM_VSTOP      8
#define TERM_VSUSP      9
#define TERM_VTIME      10

namespace kernel {
    // Termios mode flags
    typedef u32_t termios_flag_t;
    // Termios control characters
    typedef char termios_cc_t;

    #define CC_COUNT 11

    struct TermiosData {
        termios_flag_t f_input_flags;
        termios_flag_t f_output_flags;
        termios_flag_t f_control_flags;
        termios_flag_t f_local_flags;
        termios_cc_t f_control_characters[CC_COUNT];
    };

    class TermiosHelper {
    public:
        typedef bool char_write_cb_t(void* cbArg, u8_t c);

    private:
        TermiosData f_termios;
        char_write_cb_t* f_write_cb;
        void* f_arg;

        bool f_output_paused;

        size_t f_write_head;
        size_t f_line_head;
        size_t f_read_head;
        u8_t* f_input_buffer;

    public:
        TermiosHelper(char_write_cb_t* writeCallback, void* callbackArg);
        ~TermiosHelper();

        /**
         * @brief Get termios data struct
         *
         * @return Pointer to termios data
         */
        TermiosData* termios();

        /**
         * @brief Handle received character
         *
         * This method is called by the driver when a character is received (i.e. from a serial port).
         * The actual character is passed in the lower 8 bits of the argument. The upper 8 bits are
         * status bits to which indicate the following conditions:
         *  bit 0 - parity error
         *  bit 1 - break condition
         *
         * @param c The character that was received
         */
        void character_received(u16_t c);

        /**
         * @brief Stream read
         *
         * This method is supposed to be called by the read syscall.
         *
         * @param buffer Read data destination
         * @param length Amount to read
         *
         * @return Amount actually read
         */
        size_t stream_read(void* buffer, size_t length);

        /**
         * @brief Stream write
         *
         * This method is supposed to be called by the write syscall.
         *
         * @param buffer Write data source
         * @param length Amount to write
         *
         * @return Amount actually written
         */
        size_t stream_write(const void* buffer, size_t length);

    private:
        bool handle_conditions(u16_t c);
        size_t translate_character_input(u16_t c, char* result);
        bool handle_canon(u16_t c);
        bool handle_signal(u16_t c);
        bool handle_flow(u16_t c);

        void write_input(char c);
        char read_input();

        void write_output(char c);
    };
}
