#include "inode.hpp"
#include <fs/devicefs.hpp>

using namespace kernel;

INodePtr read_inode(MountInfo& mi, u32_t inode_idx) {
    u32_t inode_group = mi.get_inode_group(inode_idx);
    u32_t inode_table_block = mi.block_groups[inode_group].inode_table;

    u32_t inode_block = mi.get_inode_block(inode_idx) + inode_table_block;
    CacheBlock* cb = mi.read_cache_block(inode_block);

    u32_t inode_offset = ((inode_idx - 1) % (mi.block_size / mi.superblock->ext_inode_size)) * mi.superblock->ext_inode_size;

    INodePtr ptr = INodePtr(cb, inode_offset);
    return ptr;
}

INodePtr::INodePtr(CacheBlock* block, u32_t offset)
    : f_block(block)
    , f_offset(offset) {
    f_block->ref();
}

INodePtr::~INodePtr() {
    if(f_block != 0) f_block->unref();
}

INodePtr::INodePtr(const INodePtr& other)
    : f_block(other.f_block)
    , f_offset(other.f_offset) {
    f_block->ref();
}

INodePtr::INodePtr(INodePtr&& other)
    : f_block(other.leak_ptr())
    , f_offset(other.f_offset) {
}

INodePtr& INodePtr::operator=(const INodePtr& other) {
    f_block->unref();

    f_block = other.f_block;
    f_offset = other.f_offset;

    f_block->ref();

    return *this;
}

INodePtr& INodePtr::operator=(INodePtr&& other) {
    f_block->unref();

    f_block = other.leak_ptr();
    f_offset = other.f_offset;

    return *this;
}

u32_t get_inode_block(MountInfo& mi, INodePtr& inode, u32_t inode_block_index) {
    u32_t ptr_per_block = mi.block_size / 4;
    if(inode_block_index < 12) {
        // Direct
        return inode->direct[inode_block_index];
    } else if(inode_block_index < 12 + ptr_per_block) {
        // Singly
        auto* cb = mi.read_cache_block(inode->singly_indirect);
        return ((u32_t*)cb->ptr())[ptr_per_block - 12];
    } else if(inode_block_index < 12 + ptr_per_block + ptr_per_block * ptr_per_block) {
        // Doubly
        auto* cb_singly = mi.read_cache_block(inode->doubly_indirect);
        u32_t local_index = ptr_per_block - 12 - ptr_per_block;
        auto* cb = mi.read_cache_block(((u32_t*)cb_singly->ptr())[local_index / ptr_per_block]);
        return ((u32_t*)cb->ptr())[local_index % ptr_per_block];
    } else {
        // Triply
        auto* cb_doubly = mi.read_cache_block(inode->triply_indirect);
        u32_t local_index = ptr_per_block - 12 - ptr_per_block - ptr_per_block * ptr_per_block;
        auto* cb_singly = mi.read_cache_block(((u32_t*)cb_doubly->ptr())[local_index / (ptr_per_block * ptr_per_block)]);
        auto* cb = mi.read_cache_block(((u32_t*)cb_singly->ptr())[local_index / ptr_per_block % ptr_per_block]);
        return ((u32_t*)cb->ptr())[local_index % ptr_per_block];
    }
}
