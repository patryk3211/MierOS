#include <types.h>
#include <streams/termios.hpp>
#include <stdlib.h>

using namespace kernel;

TermiosHelper::TermiosHelper(char_write_cb_t* writeCallback, void* callbackArg) {
    f_write_cb = writeCallback;
    f_arg = callbackArg;

    f_read_head = 0;
    f_write_head = 0;
    f_line_head = 0;

    f_input_buffer = new u8_t[4096];
    
    memset(&f_termios, 0, sizeof(f_termios));

    // Initialize default termios values
    f_termios.c_cc[VEOF] =     004;
    f_termios.c_cc[VERASE] =   0177;
    f_termios.c_cc[VINTR] =    003;
    f_termios.c_cc[VKILL] =    025;
    f_termios.c_cc[VQUIT] =    034;
    f_termios.c_cc[VSTART] =   021;
    f_termios.c_cc[VSTOP] =    023;
    f_termios.c_cc[VSUSP] =    032;
}

TermiosHelper::~TermiosHelper() {
    
}

termios* TermiosHelper::termios() {
    return &f_termios;
}

#define MATCH_CC(cc, c) f_termios.c_cc[(cc)] && f_termios.c_cc[(cc)] == (c)

void TermiosHelper::character_received(u16_t data) {
    u8_t character = data & 0xFF;

    write_output(character);

    if(!handle_conditions(data))
        return;

    char output[4];
    size_t length = translate_character_input(data, output);

    // Use the last character for modes
    character = output[length - 1];

    // Canonical mode
    if(f_termios.c_lflag & ICANON) {
        if(!handle_canon(character))
            return;
    }

    // Signals
    if(f_termios.c_lflag & ISIG) {
        if(!handle_signal(character))
            return;
    }

    // Flow control
    if(f_termios.c_iflag & IXON) {
        if(!handle_flow(character))
            return;
    }

    for(size_t i = 0; i < length; ++i)
        write_input(output[i]);
}

size_t TermiosHelper::stream_read(void* buffer, size_t length) {
    if(f_termios.c_lflag & ICANON) {
        // Wait for line
        /// TODO: [02.02.2023] Take TIME and MIN settings into account
        while(f_line_head == f_read_head);
    }

    char* charBuffer = (char*)buffer;

    for(size_t i = 0; i < length; ++i) {
        if(f_line_head == f_read_head) {
            // Ran out of data
            return i;
        }
        charBuffer[i] = read_input();
    }

    return length;
}

size_t TermiosHelper::stream_write(const void* buffer, size_t length) {
    u8_t* byteBuffer = (u8_t*)buffer;

    for(size_t i = 0; i < length; ++i) {
        u8_t c = byteBuffer[i];
        write_output(c);
    }

    return length;
}

bool TermiosHelper::handle_conditions(u16_t data) {
    // Break condition
    if(data & 0x200) {
        if(f_termios.c_iflag & IGNBRK)
            return false;
    }

    // Parity error
    if(data & 0x100) {
        if(f_termios.c_iflag & IGNPAR)
            return false;
    }

    // Ignore CR
    if((f_termios.c_iflag & IGNCR) && (data & 0xFF) == '\r')
        return false;

    return true;
}

size_t TermiosHelper::translate_character_input(u16_t c, char* result) {
    if(f_termios.c_iflag & PARENB) {
        if(f_termios.c_iflag & PARMRK) {
            // Mark parity error byte
            if(c & 0x100) {
                result[0] = 0177;
                result[1] = 0;
                result[2] = c & 0xFF;
                return 3;
            } else if(c == 0177) {
                result[0] = 0177;
                result[1] = 0177;
                return 2;
            }
        } else {
            if(c & 0x100) {
                result[0] = 0;
                return 1;
            }
        }
    }

    if(!(f_termios.c_iflag & BRKINT) && (f_termios.c_iflag & PARMRK) && (c & 0x200)) {
        result[0] = 0177;
        result[1] = 0;
        result[2] = 0;
        return 3;
    }

    if(c == '\n' && f_termios.c_iflag & INLCR) {
        result[0] = '\r';
        return 1;
    }

    if(c == '\r' && f_termios.c_iflag & ICRNL) {
        result[0] = '\n';
        return 1;
    }

    u8_t mask = (f_termios.c_iflag & ISTRIP) ? 0x7F : 0xFF;
    result[0] = c & mask;
    return 1;
}

bool TermiosHelper::handle_canon(u16_t c) {
    u8_t character = c & 0xFF;

    if(MATCH_CC(VEOF, character)) {
        // Output a line delimiter and pass the line to process
        write_input('\r');
        f_line_head = f_write_head;
        return false;
    } else if(MATCH_CC(VEOL, character)) {
        // Pass line to process
        f_line_head = f_write_head;
        return false;
    } else if(MATCH_CC(VERASE, character)) {

    } else if(MATCH_CC(VKILL, character)) {

    } else if(character == '\r') {
        // Pass the line to process
        f_line_head = f_write_head;
        return false;
    } else if(character == '\b') {
        if(f_line_head > f_write_head) {
            write_output('\b');
            --f_write_head;
        }
        return false;
    }

    return true;
}

bool TermiosHelper::handle_signal(u16_t c) {
    u8_t character = c & 0xFF;

    if(MATCH_CC(VINTR, character)) {

    } else if(MATCH_CC(VQUIT, character)) {

    } else if(MATCH_CC(VSUSP, character)) {

    }

    return true;
}

bool TermiosHelper::handle_flow(u16_t c) {
    u8_t character = c & 0xFF;

    if(MATCH_CC(VSTART, character)) {

    } else if(MATCH_CC(VSTOP, character)) {

    }

    return true;
}

void TermiosHelper::write_input(char c) {
    // Check if we have buffer space to save the character
    if(f_write_head + 1 == f_read_head)
        return;

    f_input_buffer[f_write_head] = c;
    f_write_head = (f_write_head + 1) % 4096;
}

char TermiosHelper::read_input() {
    while(f_read_head == f_line_head);

    char out = f_input_buffer[f_read_head];
    f_read_head = (f_read_head + 1) % 4096;

    return out;
}

void TermiosHelper::write_output(char c) {
    if(f_termios.c_oflag & OPOST) {
        if(c == '\n' && (f_termios.c_oflag & ONLCR)) {
            f_write_cb(f_arg, '\r');
        }
        if(c == '\r') {
            if(f_termios.c_oflag & ONLRET)
                return;

            if(f_termios.c_oflag & ONOCR) {
                /// TODO: [01.02.2023] We need to keep track of columns for this
                /// flag, for now leave this unimplemented.
            }

            if(f_termios.c_oflag & OCRNL) {
                f_write_cb(f_arg, '\n');
                return;
            }
        }
    }
    f_write_cb(f_arg, c);
}

void TermiosHelper::flush_input() {
    f_read_head = 0;
    f_write_head = 0;
    f_line_head = 0;
}

