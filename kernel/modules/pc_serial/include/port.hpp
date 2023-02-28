#pragma once

#include "asm/termios.h"
#include "fifo.hpp"
#include <fs/filesystem.hpp>
#include <streams/termios.hpp>

#define SET_IMMEDIATE   1
#define SET_DRAIN       2
#define SET_FLUSH       4

struct Port {
    u16_t ioPort;
    u32_t openCount;

    FIFO outputBuffer;
    //FIFO inputBuffer;
    bool transmitting;

    kernel::TermiosHelper termiosHelper;
    termios pendingSettings;
    int setWhen;

    kernel::VNodePtr deviceNode;

    Port(u16_t ioPort, size_t bufferCapacity, kernel::TermiosHelper::char_write_cb_t* writeCallback)
        : ioPort(ioPort)
        , outputBuffer(bufferCapacity)
        , termiosHelper(writeCallback, this)
        , deviceNode(nullptr) {
        openCount = 0;
        setWhen = 0;
        transmitting = false;
    }

    void set_termios(void* ptr, int when);
};

