#pragma once

#include <errno.h>
#include <fs/filesystem.hpp>
#include <fs/vnode.hpp>

namespace kernel {
    struct ModuleVNodeDataStorage : public VNodeDataStorage {
        u16_t major;

        ModuleVNodeDataStorage(u16_t major);
        ~ModuleVNodeDataStorage();
    };

    class ModuleFilesystem : public Filesystem {
        u16_t major;
        u16_t minor;

    public:
        ModuleFilesystem(u16_t major, u16_t minor);
        virtual ~ModuleFilesystem();

        virtual ValueOrError<void> umount();

        virtual ValueOrError<std::SharedPtr<VNode>> get_file(std::SharedPtr<VNode> root, const char* path, FilesystemFlags flags);
        virtual ValueOrError<std::List<std::SharedPtr<VNode>>> get_files(std::SharedPtr<VNode> root, const char* path, FilesystemFlags flags);

        virtual ValueOrError<VNodePtr> resolve_link(VNodePtr link);

        virtual ValueOrError<void> open(FileStream* stream, int mode);
        virtual ValueOrError<void> close(FileStream* stream);

        virtual ValueOrError<size_t> read(FileStream* stream, void* buffer, size_t length);
        virtual ValueOrError<size_t> write(FileStream* stream, const void* buffer, size_t length);

        virtual ValueOrError<size_t> seek(FileStream* stream, size_t position, int mode);
    };
}
