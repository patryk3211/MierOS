#pragma once

#include <types.h>
#include <defines.h>

struct Superblock {
    // Base superblock fields
    u32_t total_inodes;
    u32_t total_blocks;
    u32_t reserved_blocks;
    u32_t free_blocks;
    u32_t free_inodes;
    u32_t superblock_block;
    u32_t blocks_size;
    u32_t fragment_size;
    u32_t group_block_count;
    u32_t group_fragment_count;
    u32_t group_inode_count;
    u32_t last_mount_time;
    u32_t last_write_time;
    u16_t last_check_mount_count;
    u16_t max_mount_before_check;
    u16_t signature;
    u16_t fs_state;
    u16_t error_action;
    u16_t version_minor;
    u32_t last_check_time;
    u32_t time_before_check;
    u32_t creator_os_id;
    u32_t version_major;
    u16_t reserved_owner_uid;
    u16_t reserved_owner_gid;

    // Extended superblock fields
    u32_t ext_first_inode;
    u16_t ext_inode_size;
    u16_t ext_superblock_group;
    u32_t ext_optional_features;
    u32_t ext_required_features;
    u32_t ext_readonly_features;
    u8_t ext_filesystem_id[16];
    u8_t ext_volume_name[16];
    u8_t ext_last_mount_path[64];
    u32_t ext_compression_algorithm;
    u8_t ext_preallocate_block_count_file;
    u8_t ext_preallocate_block_count_dir;
    u16_t ext_unused;
    u8_t ext_journal_id[16];
    u32_t ext_journal_inode;
    u32_t ext_journal_device;
    u32_t ext_head_of_orphan_list;

    u8_t reserved[788];
} PACKED;
