#include "data_storage.hpp"
#include "fs_func.hpp"

Ext2VNodeDataStorage::~Ext2VNodeDataStorage() {

}

void fs_data_destroy(kernel::ModuleVNodeDataStorage& data_obj) {
    static_cast<Ext2VNodeDataStorage&>(data_obj).~Ext2VNodeDataStorage();
}
