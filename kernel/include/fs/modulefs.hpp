#pragma once

#include <fs/filesystem.hpp>
#include <errno.h>

namespace kernel {
    class ModuleFilesystem : public Filesystem {
        u16_t major;
        u16_t minor;
    public:
        ModuleFilesystem(u16_t major, u16_t minor);
        virtual ~ModuleFilesystem();

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
