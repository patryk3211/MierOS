#pragma once

#include <fs/filesystem.hpp>
#include <pair.hpp>

namespace kernel {
    struct DeviceFunctionTable {
        ValueOrError<void> (*open)(u16_t minor, FileStream* stream, int mode);
        ValueOrError<void> (*close)(u16_t minor, FileStream* stream);

        ValueOrError<size_t> (*read)(u16_t minor, FileStream* stream, void* buffer, size_t length);
        ValueOrError<size_t> (*write)(u16_t minor, FileStream* stream, const void* buffer, size_t length);

        ValueOrError<size_t> (*seek)(u16_t minor, FileStream* stream, size_t position, int mode);

        ValueOrError<u32_t> (*block_read)(u16_t minor, u64_t lba, u32_t sector_count, void* buffer);
        ValueOrError<u32_t> (*block_write)(u16_t minor, u64_t lba, u32_t sector_count, const void* buffer);

        ValueOrError<int> (*ioctl)(u16_t minor, u64_t request, void* arg);
    };

    class DeviceFilesystem : public Filesystem {
        static DeviceFilesystem* s_instance;

        VNodePtr root;

    public:
        DeviceFilesystem();
        virtual ~DeviceFilesystem() { }

        virtual ValueOrError<VNodePtr> get_file(VNodePtr root, const char* filename, FilesystemFlags flags);
        virtual ValueOrError<std::List<VNodePtr>> get_files(VNodePtr root, FilesystemFlags flags);

        virtual ValueOrError<VNodePtr> resolve_link(VNodePtr link);

        virtual ValueOrError<void> open(FileStream* stream, int mode);
        virtual ValueOrError<void> close(FileStream* stream);

        virtual ValueOrError<size_t> read(FileStream* stream, void* buffer, size_t length);
        virtual ValueOrError<size_t> write(FileStream* stream, const void* buffer, size_t length);

        virtual ValueOrError<size_t> seek(FileStream* stream, size_t position, int mode);

        virtual ValueOrError<int> ioctl(FileStream* stream, u64_t request, void* arg);

        ValueOrError<u32_t> block_read(VNodePtr bdev, u64_t lba, u32_t sector_count, void* buffer);
        ValueOrError<u32_t> block_write(VNodePtr bdev, u64_t lba, u32_t sector_count, const void* buffer);

        ValueOrError<VNodePtr> add_dev(const char* path, u16_t major, u16_t minor);
        ValueOrError<VNodePtr> add_link(const char* path, VNodePtr destination);

        static DeviceFilesystem* instance() { return s_instance; }

    private:
        std::Pair<u16_t, DeviceFunctionTable*> get_function_table(VNodePtr node);
    };
}
