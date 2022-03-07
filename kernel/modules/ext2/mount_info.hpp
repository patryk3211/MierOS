#pragma once

#include <fs/vnode.hpp>
#include "superblock.hpp"
#include <memory/kbuffer.hpp>

struct MountInfo {
    kernel::VNode* fs_file;
    kernel::TypedKBuffer<Superblock> superblock;
    u32_t block_group_count;
    u32_t block_size;
    bool sb_ext;
};
