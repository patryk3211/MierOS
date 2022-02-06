#pragma once

#include <streams/stream.hpp>

namespace kernel {
    #define STREAM_TYPE_FILE 1

    class VNode;
    class FileStream : public Stream {
        VNode* f_file;

        FileStream(VNode* file) : Stream(STREAM_TYPE_FILE), f_file(file) { }
    public:

        friend class VNode;
    };
}
