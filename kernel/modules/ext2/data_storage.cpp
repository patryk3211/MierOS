#include "data_storage.hpp"
#include "fs_func.hpp"

using namespace kernel;

extern u16_t mod_major;

Ext2VNodeDataStorage::Ext2VNodeDataStorage(INodePtr& inode)
    : ModuleVNodeDataStorage(mod_major)
    , inode(inode) {
}

Ext2VNodeDataStorage::~Ext2VNodeDataStorage() {
}

void fs_data_destroy(ModuleVNodeDataStorage& data_obj) {
    static_cast<Ext2VNodeDataStorage&>(data_obj).~Ext2VNodeDataStorage();
}

Ext2StreamDataStorage::Ext2StreamDataStorage(MountInfo& mi)
    : buffer(mi.block_size) {
    position = 0;
}
