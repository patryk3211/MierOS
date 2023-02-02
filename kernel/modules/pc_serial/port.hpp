#pragma once

#include "fifo.hpp"
#include <streams/termios.hpp>

struct Port {
    u16_t ioPort;
    u32_t openCount;

    FIFO outputBuffer;
    //FIFO inputBuffer;

    kernel::TermiosHelper termiosHelper;

    Port(u16_t ioPort, size_t bufferCapacity, kernel::TermiosHelper::char_write_cb_t* writeCallback)
        : ioPort(ioPort)
        , outputBuffer(bufferCapacity)
        , termiosHelper(writeCallback, this) {
        //, inputBuffer(bufferCapacity) {
        openCount = 0;
    }
};

