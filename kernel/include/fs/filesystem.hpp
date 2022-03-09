#pragma once

#include <types.h>
#include <list.hpp>
#include <errno.h>
#include <shared_pointer.hpp>

namespace kernel {
    class FileStream;
    class VNode;

    struct FilesystemFlags {
        /**
         * @brief If the target file is a link the return value will be the node pointed to by this link
         * 
         */
        bool resolve_link:1;
        
        /**
         * @brief Resolve links in the path, if this value is false will return ERR_LINK if a link is found
         * 
         */
        bool follow_links:1;
    };

    class Filesystem {
    protected:
        Filesystem() { }

    public:
        virtual ~Filesystem() = default;

        virtual ValueOrError<void> umount();

        virtual ValueOrError<std::SharedPtr<VNode>> get_file(std::SharedPtr<VNode> root, const char* path, FilesystemFlags flags);
        virtual ValueOrError<std::List<std::SharedPtr<VNode>>> get_files(std::SharedPtr<VNode> root, const char* path, FilesystemFlags flags);
    
        virtual ValueOrError<void> open(FileStream* stream, int mode);
        virtual ValueOrError<void> close(FileStream* stream);

        virtual ValueOrError<size_t> read(FileStream* stream, void* buffer, size_t length);
        virtual ValueOrError<size_t> write(FileStream* stream, const void* buffer, size_t length);

        virtual ValueOrError<size_t> seek(FileStream* stream, size_t position, int mode);
    };
}
