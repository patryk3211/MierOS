#pragma once

#include "fifo.hpp"
#include <fs/filesystem.hpp>
#include <streams/termios.hpp>

struct Port {
    u16_t ioPort;
    u32_t openCount;

    FIFO outputBuffer;
    //FIFO inputBuffer;

    kernel::TermiosHelper termiosHelper;

    kernel::VNodePtr deviceNode;

    Port(u16_t ioPort, size_t bufferCapacity, kernel::TermiosHelper::char_write_cb_t* writeCallback)
        : ioPort(ioPort)
        , outputBuffer(bufferCapacity)
        , termiosHelper(writeCallback, this)
        , deviceNode(nullptr) {
        //, inputBuffer(bufferCapacity) {
        openCount = 0;
    }
};

