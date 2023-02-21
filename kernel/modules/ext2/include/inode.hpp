#pragma once

#include "mount_info.hpp"
#include <defines.h>
#include <shared_pointer.hpp>
#include <types.h>

#define INODE_TYPE_FIFO 0x1000
#define INODE_TYPE_CHARDEV 0x2000
#define INODE_TYPE_DIRECTORY 0x4000
#define INODE_TYPE_BLOCKDEV 0x6000
#define INODE_TYPE_FILE 0x8000
#define INODE_TYPE_SYMLINK 0xA000
#define INODE_TYPE_SOCKET 0xC000

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
    u32_t singly_indirect;
    u32_t doubly_indirect;
    u32_t triply_indirect;
    u32_t generation_num;
    u32_t extended_attributes;
    u32_t upper_size_dir_acl;
    u32_t fragment_block_address;
    u8_t os_val2[12];
} PACKED;

class INodePtr {
    CacheBlock* f_block;
    u32_t f_offset;

public:
    INodePtr(CacheBlock* block, u32_t offset);
    ~INodePtr();

    INodePtr(const INodePtr& other);
    INodePtr(INodePtr&& other);

    INodePtr& operator=(const INodePtr& other);
    INodePtr& operator=(INodePtr&& other);

    Inode* ptr() { return (Inode*)((u8_t*)f_block->ptr() + f_offset); }
    const Inode* ptr() const { return (Inode*)((u8_t*)f_block->ptr() + f_offset); }

    Inode* operator->() { return ptr(); }
    const Inode* operator->() const { return ptr(); }

    Inode& operator*() { return *ptr(); }
    const Inode& operator*() const { return *ptr(); }

private:
    CacheBlock* leak_ptr() {
        auto* r = f_block;
        f_block = 0;
        return r;
    }
};

INodePtr read_inode(MountInfo& mi, u32_t inode_index);
u32_t get_inode_block(MountInfo& mi, INodePtr& inode, u32_t inode_block_index);
