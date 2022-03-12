#pragma once

#include <fs/modulefs.hpp>
#include "inode.hpp"

struct Ext2VNodeDataStorage : public kernel::ModuleVNodeDataStorage {
    INodePtr inode;

    ~Ext2VNodeDataStorage();
};
