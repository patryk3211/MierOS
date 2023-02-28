#pragma once

#include <types.h>
#include <asm/termios.h>

namespace kernel {
    class TermiosHelper {
    public:
        typedef bool char_write_cb_t(void* cbArg, u8_t c);

    private:
        termios f_termios;
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
        termios* termios();

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

        /**
         * @brief Flush input buffer
         *
         * Discards all data pending in the input buffer
         */
        void flush_input();

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
