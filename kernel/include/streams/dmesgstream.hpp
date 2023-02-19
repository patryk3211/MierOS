#pragma once

#include <streams/stream.hpp>

#define STREAM_TYPE_DMESG 3

namespace kernel {
    class DMesgStream : public Stream {
        char f_buffer[2048];
        size_t f_buffer_pos;

    public:
        DMesgStream();
        ~DMesgStream() = default;

        virtual ValueOrError<size_t> write(const void *buffer, size_t length) override;
        virtual ValueOrError<size_t> seek(size_t position, int mode) override;
    };
}

