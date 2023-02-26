#pragma once

#include <errno.h>
#include <fs/vnode.hpp>
#include <list.hpp>
#include <streams/filestream.hpp>
#include <types.h>
#include <memory/page/resolved_memory.hpp>
#include <memory/page/resolvable_memory.hpp>

namespace kernel {
    struct ModuleVNodeDataStorage;
    struct FilesystemDriver {
        ValueOrError<u16_t> (*mount)(VNodePtr fs_file);
        ValueOrError<void> (*umount)(u16_t minor);

        void (*set_fs_object)(u16_t minor, Filesystem* fs_obj);

        ValueOrError<VNodePtr> (*get_file)(u16_t minor, VNodePtr root, const char* filename, FilesystemFlags flags);
        ValueOrError<std::List<VNodePtr>> (*get_files)(u16_t minor, VNodePtr root, FilesystemFlags flags);

        ValueOrError<VNodePtr> (*resolve_link)(u16_t minor, VNodePtr link, int depth);

        ValueOrError<void> (*open)(u16_t minor, FileStream* filestream, int mode);
        ValueOrError<void> (*close)(u16_t minor, FileStream* filestream);

        ValueOrError<size_t> (*read)(u16_t minor, FileStream* filestream, void* buffer, size_t length);
        ValueOrError<size_t> (*write)(u16_t minor, FileStream* filestream, const void* buffer, size_t length);

        ValueOrError<size_t> (*seek)(u16_t minor, FileStream* filestream, size_t position, int mode);

        std::Optional<ResolvedMemoryEntry> (*resolve_mapping)(u16_t minor, const ResolvableMemoryEntry& mapping, virtaddr_t addr);
        void (*sync_mapping)(u16_t minor, const ResolvedMemoryEntry& mapping);
    };
}
