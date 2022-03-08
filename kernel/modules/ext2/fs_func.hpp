#pragma once

#include <fs/vnode.hpp>
#include <errno.h>

kernel::ValueOrError<u16_t> mount(kernel::VNode* fs_file);
void set_fs_object(u16_t minor, kernel::Filesystem* fs_obj);
