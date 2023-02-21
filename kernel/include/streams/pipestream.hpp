#pragma once

#include <streams/stream.hpp>
#include <shared_pointer.hpp>
#include <pair.hpp>

#define STREAM_TYPE_PIPE 4

namespace kernel {
    class PipeStream : public Stream {
        struct StreamData {
            bool f_writeClosed;
            bool f_readClosed;

            u8_t f_buffer[4096];

            size_t f_writeHead;
            size_t f_readHead;

            StreamData();
        };

        std::SharedPtr<StreamData> f_data;
        bool f_isWrite;

        PipeStream(const std::SharedPtr<StreamData>& data, bool write);

    public:
        virtual ~PipeStream();

        virtual ValueOrError<size_t> read(void *buffer, size_t length) override;
        virtual ValueOrError<size_t> write(const void *buffer, size_t length) override;
        virtual ValueOrError<size_t> seek(size_t offset, int mode) override;

        static std::Pair<PipeStream*, PipeStream*> make_pair();
    };
}

