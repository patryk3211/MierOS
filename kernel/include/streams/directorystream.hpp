#pragma once

#include <streams/stream.hpp>
#include <fs/vnode.hpp>

namespace kernel {
    #define STREAM_TYPE_DIRECTORY 2
    class DirectoryStream : public Stream {
        VNodePtr f_directory;
        std::List<VNodePtr> f_directories;
        std::List<VNodePtr>::iterator f_current_entry;
        bool f_open;

    public:
        DirectoryStream(const VNodePtr& directory);
        virtual ~DirectoryStream() = default;

        ValueOrError<void> open();

        virtual ValueOrError<size_t> read(void* buffer, size_t length) override;
        virtual ValueOrError<size_t> seek(size_t offset, int mode) override;
    };
}

