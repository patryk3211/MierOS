#include "data_storage.hpp"
#include "fs_func.hpp"

using namespace kernel;

Ext2VNodeDataStorage::Ext2VNodeDataStorage(INodePtr& inode)
    : inode(inode) {
}

Ext2VNodeDataStorage::~Ext2VNodeDataStorage() {
}

Ext2StreamDataStorage::Ext2StreamDataStorage(MountInfo& mi)
    : buffer(mi.block_size) {
    position = 0;
}
