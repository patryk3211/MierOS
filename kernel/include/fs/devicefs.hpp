#pragma once

#include <fs/filesystem.hpp>

namespace kernel {
    struct DevFsFunctionTable {
        ValueOrError<void> (*open)(u16_t minor, FileStream* stream, int mode);
        ValueOrError<void> (*close)(u16_t minor, FileStream* stream);

        ValueOrError<size_t> (*read)(u16_t minor, FileStream* stream, void* buffer, size_t length);
        ValueOrError<size_t> (*write)(u16_t minor, FileStream* stream, const void* buffer, size_t length);

        ValueOrError<size_t> (*seek)(u16_t minor, FileStream* stream, size_t position, int mode);

        ValueOrError<u32_t> (*block_read)(u16_t minor, u64_t lba, u32_t sector_count, void* buffer);
        ValueOrError<u32_t> (*block_write)(u16_t minor, u64_t lba, u32_t sector_count, const void* buffer);
    };

    class DeviceFilesystem : public Filesystem {
        static DeviceFilesystem* s_instance;

        std::SharedPtr<VNode> root;
    public:
        DeviceFilesystem();
        virtual ~DeviceFilesystem() { }

        virtual ValueOrError<std::SharedPtr<VNode>> get_file(std::SharedPtr<VNode> root, const char* path, FilesystemFlags flags);
        virtual ValueOrError<std::List<std::SharedPtr<VNode>>> get_files(std::SharedPtr<VNode> root, const char* path, FilesystemFlags flags);

        virtual ValueOrError<VNodePtr> resolve_link(VNodePtr link);
    
        virtual ValueOrError<void> open(FileStream* stream, int mode);
        virtual ValueOrError<void> close(FileStream* stream);

        virtual ValueOrError<size_t> read(FileStream* stream, void* buffer, size_t length);
        virtual ValueOrError<size_t> write(FileStream* stream, const void* buffer, size_t length);

        virtual ValueOrError<size_t> seek(FileStream* stream, size_t position, int mode);

        ValueOrError<u32_t> block_read(std::SharedPtr<VNode> bdev, u64_t lba, u32_t sector_count, void* buffer);
        ValueOrError<u32_t> block_write(std::SharedPtr<VNode> bdev, u64_t lba, u32_t sector_count, const void* buffer);

        ValueOrError<std::SharedPtr<VNode>> add_dev(const char* path, u16_t major, u16_t minor);
        ValueOrError<std::SharedPtr<VNode>> add_link(const char* path, std::SharedPtr<VNode> destination);

        static DeviceFilesystem* instance() { return s_instance; }
    };
}
