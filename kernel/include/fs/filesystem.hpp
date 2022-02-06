#pragma once

#include <types.h>
#include <list.hpp>
#include <errno.h>

namespace kernel {
    class FileStream;
    class VNode;

    class Filesystem {
    protected:
        Filesystem() { }

    public:
        virtual ~Filesystem() = default;

        virtual ValueOrError<void> umount();

        virtual ValueOrError<VNode*> get_file(VNode* root, const char* path);
        virtual ValueOrError<std::List<VNode*>> get_files(VNode* root, const char* path);
    
        virtual ValueOrError<void> open(FileStream* stream, int mode);
        virtual ValueOrError<void> close(FileStream* stream);

        virtual ValueOrError<size_t> read(FileStream* stream, void* buffer, size_t length);
        virtual ValueOrError<size_t> write(FileStream* stream, void* buffer, size_t length);

        virtual ValueOrError<size_t> seek(FileStream* stream, size_t position, int mode);
    };
}
