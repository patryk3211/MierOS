#pragma once

#include <fs/vnode.hpp>
#include <errno.h>
#include <fs/modulefs.hpp>

kernel::ValueOrError<u16_t> mount(std::SharedPtr<kernel::VNode> fs_file);
void set_fs_object(u16_t minor, kernel::Filesystem* fs_obj);
void fs_data_destroy(kernel::ModuleVNodeDataStorage& data_obj);

kernel::ValueOrError<std::SharedPtr<kernel::VNode>> get_file(u16_t minor, std::SharedPtr<kernel::VNode> root, const char* path, kernel::FilesystemFlags flags);
kernel::ValueOrError<std::List<std::SharedPtr<kernel::VNode>>> get_files(u16_t minor, std::SharedPtr<kernel::VNode> root, const char* path, kernel::FilesystemFlags flags);
