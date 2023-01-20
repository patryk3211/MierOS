#pragma once

#include <fs/filesystem.hpp>

namespace kernel {
    class InitRdFilesystem : public Filesystem {
        VNodePtr f_root;

    public:
        InitRdFilesystem(void* initrd);
        virtual ~InitRdFilesystem() = default;

        virtual ValueOrError<void> umount();

        virtual ValueOrError<VNodePtr> get_file(VNodePtr root, const char* path, FilesystemFlags flags);
        virtual ValueOrError<std::List<VNodePtr>> get_files(VNodePtr root, const char* path, FilesystemFlags flags);

        virtual ValueOrError<void> open(FileStream* stream, int mode);
        virtual ValueOrError<void> close(FileStream* stream);

        virtual ValueOrError<size_t> read(FileStream* stream, void* buffer, size_t length);
        virtual ValueOrError<size_t> write(FileStream* stream, const void* buffer, size_t length);

        virtual ValueOrError<size_t> seek(FileStream* stream, size_t position, int mode);

    private:
        ValueOrError<VNodePtr> get_node(VNodePtr root, const char* file);
    };
}
