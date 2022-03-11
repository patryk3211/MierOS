#include "inode.hpp"
#include <fs/devicefs.hpp>

using namespace kernel;

INodePtr read_inode(MountInfo mi, u32_t inode_idx) {
    u32_t inode_group = mi.get_inode_group(inode_idx);
    u32_t inode_table_block = mi.block_groups[inode_group].inode_table;
    
    u32_t inode_block = mi.get_inode_block(inode_idx) + inode_table_block;
    auto cache = mi.cache_block_table.at(inode_block);
    CacheBlock* cb = 0;
    if(cache) cb = &*cache;
    else {
        mi.cache_block_table.insert({ inode_block, { mi, inode_block } });
    }
}

INodePtr::INodePtr(CacheBlock* block, u32_t offset) {

}

INodePtr::~INodePtr() {

}
