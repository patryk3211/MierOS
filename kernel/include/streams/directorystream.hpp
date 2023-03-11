#pragma once

#include <streams/filestream.hpp>
#include <fs/vnode.hpp>

namespace kernel {
    class DirectoryStream : public FileStream {
        std::List<VNodePtr> f_directories;
        std::List<VNodePtr>::iterator f_current_entry;

    public:
        DirectoryStream(const VNodePtr& directory);
        virtual ~DirectoryStream() = default;

        ValueOrError<void> open();

        virtual ValueOrError<size_t> read(void* buffer, size_t length) override;
        virtual ValueOrError<size_t> write(const void* buffer, size_t length) override;
        virtual ValueOrError<size_t> seek(size_t offset, int mode) override;
    };
}

