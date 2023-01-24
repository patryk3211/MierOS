#pragma once

#include <errno.h>
#include <fs/filesystem.hpp>
#include <fs/vnode.hpp>
#include <fs/modulefs_functions.hpp>

namespace kernel {
    class ModuleFilesystem;

    class ModuleFilesystem : public Filesystem {
        FilesystemDriver* f_driver;
        u16_t f_major;
        u16_t f_minor;

    public:
        ModuleFilesystem(u16_t major, u16_t minor);
        virtual ~ModuleFilesystem();

        virtual ValueOrError<void> umount();

        virtual ValueOrError<VNodePtr> get_file(VNodePtr root, const char* filename, FilesystemFlags flags);
        virtual ValueOrError<std::List<VNodePtr>> get_files(VNodePtr root, FilesystemFlags flags);

        virtual ValueOrError<VNodePtr> resolve_link(VNodePtr link);

        virtual ValueOrError<void> open(FileStream* stream, int mode);
        virtual ValueOrError<void> close(FileStream* stream);

        virtual ValueOrError<size_t> read(FileStream* stream, void* buffer, size_t length);
        virtual ValueOrError<size_t> write(FileStream* stream, const void* buffer, size_t length);

        virtual ValueOrError<size_t> seek(FileStream* stream, size_t position, int mode);

        virtual PhysicalPage resolve_mapping(const FilePage& mapping, virtaddr_t addr);
        virtual void sync_mapping(const MemoryFilePage& mapping);

        FilesystemDriver* get_driver();
    };
}
