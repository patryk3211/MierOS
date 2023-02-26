#pragma once

#include <errno.h>
#include <fs/modulefs.hpp>
#include <fs/vnode.hpp>

kernel::ValueOrError<u16_t> mount(kernel::VNodePtr fs_file);
void set_fs_object(u16_t minor, kernel::Filesystem* fs_obj);

kernel::ValueOrError<kernel::VNodePtr> get_file(u16_t minor, kernel::VNodePtr root, const char* filename, kernel::FilesystemFlags flags);
kernel::ValueOrError<std::List<kernel::VNodePtr>> get_files(u16_t minor, kernel::VNodePtr root, kernel::FilesystemFlags flags);

kernel::ValueOrError<void> open(u16_t minor, kernel::FileStream* filestream, int mode);
kernel::ValueOrError<void> close(u16_t minor, kernel::FileStream* filestream);

kernel::ValueOrError<size_t> read(u16_t minor, kernel::FileStream* filestream, void* buffer, size_t length);
kernel::ValueOrError<size_t> write(u16_t minor, kernel::FileStream* filestream, const void* buffer, size_t length);

kernel::ValueOrError<size_t> seek(u16_t minor, kernel::FileStream* filestream, size_t position, int mode);

std::Optional<kernel::ResolvedMemoryEntry> resolve_mapping(u16_t minor, const kernel::ResolvableMemoryEntry& mapping, virtaddr_t addr);
