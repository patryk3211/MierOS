#pragma once

#include <types.h>
#include <defines.h>
#include <shared_pointer.hpp>

#define INODE_TYPE_FIFO       0x1000
#define INODE_TYPE_CHARDEV    0x2000
#define INODE_TYPE_DIRECTORY  0x4000
#define INODE_TYPE_BLOCKDEV   0x6000
#define INODE_TYPE_FILE       0x8000
#define INODE_TYPE_SYMLINK    0xA000
#define INODE_TYPE_SOCKET     0xC000

struct Inode {
    u16_t type_and_perm;
    u16_t user_id;
    u32_t size;
    u32_t access_time;
    u32_t create_time;
    u32_t modify_time;
    u32_t delete_time;
    u16_t group_id;
    u16_t hardlink_count;
    u32_t sector_count;
    u32_t flags;
    u32_t os_val1;
    u32_t direct[12];
    u32_t signly_indirect;
    u32_t doubly_indirect;
    u32_t triply_indirect;
    u32_t generation_num;
    u32_t extended_attributes;
    u32_t upper_size_dir_acl;
    u32_t fragment_block_address;
    u8_t os_val2[12];
} PACKED;

std::SharedPtr<Inode> read_inode(u32_t inode_index);