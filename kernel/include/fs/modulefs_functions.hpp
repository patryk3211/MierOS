#pragma once

#include <errno.h>
#include <fs/vnode.hpp>
#include <list.hpp>
#include <streams/filestream.hpp>
#include <types.h>
#include <memory/page/filepage.hpp>
#include <memory/page/memoryfilepage.hpp>

namespace kernel {
    struct ModuleVNodeDataStorage;
    struct fs_function_table {
        ValueOrError<u16_t> (*mount)(std::SharedPtr<VNode> fs_file);
        ValueOrError<void> (*umount)(u16_t minor);

        void (*set_fs_object)(u16_t minor, Filesystem* fs_obj);
        void (*fs_data_destroy)(ModuleVNodeDataStorage& data_obj);

        ValueOrError<std::SharedPtr<VNode>> (*get_file)(u16_t minor, std::SharedPtr<VNode> root, const char* path, FilesystemFlags flags);
        ValueOrError<std::List<std::SharedPtr<VNode>>> (*get_files)(u16_t minor, std::SharedPtr<VNode> root, const char* path, FilesystemFlags flags);

        ValueOrError<VNodePtr> (*resolve_link)(u16_t minor, VNodePtr link);

        ValueOrError<void> (*open)(u16_t minor, FileStream* filestream, int mode);
        ValueOrError<void> (*close)(u16_t minor, FileStream* filestream);

        ValueOrError<size_t> (*read)(u16_t minor, FileStream* filestream, void* buffer, size_t length);
        ValueOrError<size_t> (*write)(u16_t minor, FileStream* filestream, const void* buffer, size_t length);

        ValueOrError<size_t> (*seek)(u16_t minor, FileStream* filestream, size_t position, int mode);

        PhysicalPage (*resolve_mapping)(u16_t minor, const FilePage& mapping, virtaddr_t addr);
        void (*sync_mapping)(u16_t minor, const MemoryFilePage& mapping);
    };
}
