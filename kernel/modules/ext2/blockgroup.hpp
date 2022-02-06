#pragma once

#include <types.h>
#include <defines.h>

struct BlockGroup {
    u32_t block_usage_bitmap;
    u32_t inode_usage_bitmap;
    u32_t inode_table;
    u16_t free_blocks;
    u16_t free_inodes;
    u16_t directory_count;

    u8_t unused[14];
} PACKED;
