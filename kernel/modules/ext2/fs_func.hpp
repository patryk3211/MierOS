#pragma once

#include <fs/vnode.hpp>
#include <errno.h>
#include <fs/modulefs.hpp>

kernel::ValueOrError<u16_t> mount(std::SharedPtr<kernel::VNode> fs_file);
void set_fs_object(u16_t minor, kernel::Filesystem* fs_obj);
void fs_data_destroy(kernel::ModuleVNodeDataStorage& data_obj);
