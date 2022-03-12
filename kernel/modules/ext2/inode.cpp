#include "inode.hpp"
#include <fs/devicefs.hpp>

using namespace kernel;

INodePtr read_inode(MountInfo& mi, u32_t inode_idx) {
    u32_t inode_group = mi.get_inode_group(inode_idx);
    u32_t inode_table_block = mi.block_groups[inode_group].inode_table;

    u32_t inode_block = mi.get_inode_block(inode_idx) + inode_table_block;
    auto cache = mi.cache_block_table.at(inode_block);
    CacheBlock* cb = 0;
    if(cache) cb = *cache;
    else {
        cb = new CacheBlock(mi, inode_block);
        mi.cache_block_table.insert({ inode_block, cb });
    }
    u32_t inode_offset = ((inode_idx - 1) % (mi.block_size / mi.superblock->ext_inode_size)) * mi.superblock->ext_inode_size;

    INodePtr ptr = INodePtr(cb, inode_offset);
    return ptr;
}

INodePtr::INodePtr(CacheBlock* block, u32_t offset) : f_block(block), f_offset(offset) {
    f_block->ref();
}

INodePtr::~INodePtr() {
    if(f_block != 0) f_block->unref();
}

INodePtr::INodePtr(const INodePtr& other) : f_block(other.f_block), f_offset(other.f_offset) {
    f_block->ref();
}

INodePtr::INodePtr(INodePtr&& other) : f_block(other.leak_ptr()), f_offset(other.f_offset) {

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