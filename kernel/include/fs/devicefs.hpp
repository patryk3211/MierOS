#pragma once

#include <fs/filesystem.hpp>

namespace kernel {
    struct DevFsFunctionTable {
        ValueOrError<void> (*open)(u16_t minor, FileStream* stream, int mode);
        ValueOrError<void> (*close)(u16_t minor, FileStream* stream);

        ValueOrError<size_t> (*read)(u16_t minor, FileStream* stream, void* buffer, size_t length);
        ValueOrError<size_t> (*write)(u16_t minor, FileStream* stream, const void* buffer, size_t length);

        ValueOrError<size_t> (*seek)(u16_t minor, FileStream* stream, size_t position, int mode);
    };

    class DeviceFilesystem : public Filesystem {
        static DeviceFilesystem* s_instance;

        VNode* root;
    public:
        DeviceFilesystem();
        virtual ~DeviceFilesystem() { }

        virtual ValueOrError<void> umount();

        virtual ValueOrError<VNode*> get_file(VNode* root, const char* path, FilesystemFlags flags);
        virtual ValueOrError<std::List<VNode*>> get_files(VNode* root, const char* path, FilesystemFlags flags);
    
        virtual ValueOrError<void> open(FileStream* stream, int mode);
        virtual ValueOrError<void> close(FileStream* stream);

        virtual ValueOrError<size_t> read(FileStream* stream, void* buffer, size_t length);
        virtual ValueOrError<size_t> write(FileStream* stream, const void* buffer, size_t length);

        virtual ValueOrError<size_t> seek(FileStream* stream, size_t position, int mode);

        ValueOrError<VNode*> add_dev(const char* path, u16_t major, u16_t minor);

        static DeviceFilesystem* instance() { return s_instance; }
    };
}
