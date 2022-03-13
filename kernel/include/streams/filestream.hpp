#pragma once

#include <streams/stream.hpp>
#include <shared_pointer.hpp>
#include <errno.h>

namespace kernel {
    #define STREAM_TYPE_FILE 1

    class VNode;
    class FileStream : public Stream {
        std::SharedPtr<VNode> f_file;
        bool f_open;

    public:
        void* fs_data;

        FileStream(const std::SharedPtr<VNode>& file);
        ~FileStream();

        ValueOrError<void> open(int mode);

        std::SharedPtr<VNode>& node() { return f_file; }
    };
}
