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
    kernel::Filesystem* filesystem;

    // Get the LBA address of this block
    u64_t get_lba(u32_t block) {
        return block * block_size / 512; /// TODO: [08.03.2022] I don't think that the sector size should be hardcoded.
    }

    // Get starting block address of a group
    u32_t get_group_block(u32_t group_index) {
        return superblock->group_block_count * group_index + (superblock->blocks_size == 0 ? 1 : 0);
    }

    // Get the group that this inode belongs to
    u32_t get_inode_group(u32_t inode_index) {
        return (inode_index - 1) / superblock->group_inode_count;
    }

    // Get the group local index of this inode
    u32_t get_inode_local_index(u32_t inode_index) {
        return (inode_index - 1) % superblock->group_inode_count;
    }

    // Get the block address of this inode
    u32_t get_inode_block(u32_t inode_index) {
        return get_inode_local_index(inode_index) * superblock->ext_inode_size / block_size;
    }
};
