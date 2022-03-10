#include "inode.hpp"
#include <fs/devicefs.hpp>

using namespace kernel;

std::SharedPtr<Inode> read_inode(MountInfo mi, u32_t inode_idx) {
    u32_t inode_group = mi.get_inode_group(inode_idx);
    u32_t inode_table_block = mi.block_groups[inode_group].inode_table;
    
    mi.get_inode_block(inode_idx);
}
