#pragma once

#include "inode.hpp"
#include <fs/modulefs.hpp>

struct Ext2VNodeDataStorage : public kernel::ModuleVNodeDataStorage {
    INodePtr inode;

    Ext2VNodeDataStorage(INodePtr& inode);
    ~Ext2VNodeDataStorage();
};

struct Ext2StreamDataStorage {
    u64_t position;
    kernel::KBuffer buffer;

    Ext2StreamDataStorage(MountInfo& mi);
    ~Ext2StreamDataStorage() = default;
};
