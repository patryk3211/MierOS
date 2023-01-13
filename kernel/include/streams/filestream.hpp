#pragma once

#include <errno.h>
#include <shared_pointer.hpp>
#include <streams/stream.hpp>

#define STREAM_TYPE_FILE 1

namespace kernel {
    class VNode;
    class FileStream : public Stream {
        std::SharedPtr<VNode> f_file;
        bool f_open;

    public:
        void* fs_data;

        FileStream(const std::SharedPtr<VNode>& file);
        ~FileStream();

        ValueOrError<void> open(int mode);

        virtual size_t read(void* buffer, size_t length);

        virtual size_t seek(size_t position, int mode);

        std::SharedPtr<VNode>& node() { return f_file; }
    };
}
