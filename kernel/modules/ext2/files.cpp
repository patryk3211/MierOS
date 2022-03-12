#include "fs_func.hpp"
#include "mount_info.hpp"
#include "data_storage.hpp"
#include "dirent.hpp"

using namespace kernel;

extern std::UnorderedMap<u16_t, MountInfo> mounted_filesystems;

VNode::Type inode_to_vnode_type(u16_t inode_type) {
    switch(inode_type) {
        case INODE_TYPE_CHARDEV: 
        case INODE_TYPE_BLOCKDEV:
            return VNode::DEVICE;
        case INODE_TYPE_DIRECTORY:
            return VNode::DIRECTORY;
        case INODE_TYPE_FILE:
            return VNode::FILE;
        case INODE_TYPE_SYMLINK:
            return VNode::LINK;
    }
    panic("Unknown inode type");
}

ValueOrError<std::SharedPtr<VNode>> get_file(u16_t minor, std::SharedPtr<VNode> root, const char* path, FilesystemFlags flags) {
    auto mi_opt = mounted_filesystems.at(minor);
    ASSERT_F(mi_opt, "No filesystem is mapped to this minor number");
    auto& mi = *mi_opt;

    if(!root) root = mi.root;
    ASSERT_F(mi.filesystem == root->filesystem(), "Using a VNode from a different filesystem");

    const char* path_ptr = path;
    const char* next_separator;
    while((next_separator = strchr(path_ptr, '/')) != 0) {
        if(root->type() != VNode::DIRECTORY) return ERR_NOT_A_DIRECTORY;

        size_t length = next_separator - path_ptr;
        if(length == 0) {
            ++path_ptr;
            continue;
        }

        char part[length+1];
        memcpy(part, path_ptr, length);
        part[length] = 0;

        /*auto next_root = get_file(root, part, { 1, 1 });
        if(next_root) root = *next_root;
        else {
            if(next_root.errno() == ERR_FILE_NOT_FOUND) {
                auto root_new = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, 0, part, VNode::DIRECTORY, this);
                root->f_children.insert({ part, root_new });
                root = root_new;
            } else return next_root.errno();
        }*/
        auto next_root = root->f_children.at(part);
        if(next_root) root = *next_root;
        else {
            // Read from disk
            auto* node_data = static_cast<Ext2VNodeDataStorage*>(root->fs_data);
            auto& inode = node_data->inode;
            for(size_t blk_idx = 0; blk_idx < inode->size >> (10 + mi.superblock->blocks_size); ++blk_idx) {
                u32_t block = get_inode_block(mi, inode, blk_idx);
                auto* cb = mi.read_cache_block(block);
                
                DirectoryEntry* dir_ent;
                for(size_t offset = 0; offset < mi.block_size; offset += dir_ent->entry_size) {
                    dir_ent = (DirectoryEntry*)((u8_t*)cb->ptr()+offset);

                    if(length != dir_ent->name_length) continue;
                    if(strncmp(part, dir_ent->name, length)) continue;

                    auto inode = read_inode(mi, dir_ent->inode);
                    VNode::Type vtype = inode_to_vnode_type(inode->type_and_perm & 0xF000);
                    /// TODO: [12.03.2022] Handle symbolic links
                    if(vtype != VNode::DIRECTORY) {
                        return ERR_NOT_A_DIRECTORY;
                    }

                    auto vnode = std::make_shared<VNode>(inode->type_and_perm & 0xFFF, inode->user_id, inode->group_id, inode->create_time, inode->access_time, inode->modify_time, inode->size, part, vtype, mi.filesystem);
                    vnode->f_parent = root;
                    vnode->fs_data = new Ext2VNodeDataStorage(inode);
                    root->f_children.insert({ part, vnode });

                    root = vnode;
                    goto found;
                }
            }
            return ERR_FILE_NOT_FOUND;
        found:;
        }

        path_ptr = next_separator+1;
    }

    auto file = root->f_children.at(path_ptr);
    if(file) return *file;
    else {
        // Read from disk
        auto* node_data = static_cast<Ext2VNodeDataStorage*>(root->fs_data);
        auto& inode = node_data->inode;
        for(size_t blk_idx = 0; blk_idx < inode->size >> (10 + mi.superblock->blocks_size); ++blk_idx) {
            u32_t block = get_inode_block(mi, inode, blk_idx);
            auto* cb = mi.read_cache_block(block);
                
            DirectoryEntry* dir_ent;
            for(size_t offset = 0; offset < mi.block_size; offset += dir_ent->entry_size) {
                dir_ent = (DirectoryEntry*)((u8_t*)cb->ptr()+offset);

                if(strlen(path_ptr) != dir_ent->name_length) continue;
                if(strncmp(path_ptr, dir_ent->name, dir_ent->name_length)) continue;

                auto inode = read_inode(mi, dir_ent->inode);
                VNode::Type vtype = inode_to_vnode_type(inode->type_and_perm & 0xF000);
                /// TODO: [12.03.2022] Handle symbolic links

                auto vnode = std::make_shared<VNode>(inode->type_and_perm & 0xFFF, inode->user_id, inode->group_id, inode->create_time, inode->access_time, inode->modify_time, inode->size, path_ptr, vtype, mi.filesystem);
                vnode->f_parent = root;
                vnode->fs_data = new Ext2VNodeDataStorage(inode);
                root->f_children.insert({ path_ptr, vnode });

                return vnode;
            }
        }
        return ERR_FILE_NOT_FOUND;
    }
}
