#include "data_storage.hpp"
#include "dirent.hpp"
#include "fs_func.hpp"
#include "mount_info.hpp"

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

ValueOrError<VNodePtr> get_file(u16_t minor, VNodePtr root, const char* filename, FilesystemFlags flags) {
    auto mi_opt = mounted_filesystems.at(minor);
    ASSERT_F(mi_opt, "No filesystem is mapped to this minor number");
    auto& mi = *mi_opt;

    if(!root) root = mi.root;
    ASSERT_F(mi.filesystem == root->filesystem(), "Using a VNode from a different filesystem");

    // Since we are here, the VFS has not found the node in children of root.
    // We are guaranteed that the filename is not a path and that it is not a relative name ('.' and '..')
    // so we can safely read the file from the disk

    auto* node_data = static_cast<Ext2VNodeDataStorage*>(root->fs_data);
    auto& inode = node_data->inode;
    for(size_t blk_idx = 0; blk_idx < inode->size >> (10 + mi.superblock->blocks_size); ++blk_idx) {
        u32_t block = get_inode_block(mi, inode, blk_idx);
        auto* cb = mi.read_cache_block(block);

        DirectoryEntry* dir_ent;
        for(size_t offset = 0; offset < mi.block_size; offset += dir_ent->entry_size) {
            dir_ent = (DirectoryEntry*)((u8_t*)cb->ptr() + offset);
            if(dir_ent->inode == 0) continue;

            if(strlen(filename) != dir_ent->name_length) continue;
            if(strncmp(filename, dir_ent->name, dir_ent->name_length)) continue;

            auto inode = read_inode(mi, dir_ent->inode);
            VNode::Type vtype = inode_to_vnode_type(inode->type_and_perm & 0xF000);
            /// TODO: [12.03.2022] Handle symbolic links

            auto vnode = std::make_shared<VNode>(inode->type_and_perm & 0xFFF, inode->user_id, inode->group_id, inode->create_time, inode->access_time, inode->modify_time, inode->size, filename, vtype, mi.filesystem);
            vnode->f_parent = root;
            vnode->fs_data = new Ext2VNodeDataStorage(inode);
            root->add_child(vnode);

            return vnode;
        }
    }
    return ENOENT;
}

ValueOrError<std::List<VNodePtr>> get_files(u16_t minor, VNodePtr root, FilesystemFlags) {
    auto mi_opt = mounted_filesystems.at(minor);
    ASSERT_F(mi_opt, "No filesystem is mapped to this minor number");
    auto& mi = *mi_opt;

    if(!root) root = mi.root;
    ASSERT_F(mi.filesystem == root->filesystem(), "Using a VNode from a different filesystem");

    std::List<VNodePtr> nodes;
    for(auto val : root->f_children)
        nodes.push_back(val.value);

    // Read from disk
    auto* node_data = static_cast<Ext2VNodeDataStorage*>(root->fs_data);
    auto& inode = node_data->inode;
    for(size_t blk_idx = 0; blk_idx < inode->size >> (10 + mi.superblock->blocks_size); ++blk_idx) {
        u32_t block = get_inode_block(mi, inode, blk_idx);
        auto* cb = mi.read_cache_block(block);

        DirectoryEntry* dir_ent;
        for(size_t offset = 0; offset < mi.block_size; offset += dir_ent->entry_size) {
            dir_ent = (DirectoryEntry*)((u8_t*)cb->ptr() + offset);
            if(dir_ent->inode == 0) continue;

            char name_cp[dir_ent->name_length + 1];
            memcpy(name_cp, dir_ent->name, dir_ent->name_length);
            name_cp[dir_ent->name_length] = 0;

            if(!strcmp(name_cp, ".") || !strcmp(name_cp, "..")) continue;

            if(root->f_children.find(name_cp) == root->f_children.end()) {
                // This node does not exist yet.
                auto new_inode = read_inode(mi, dir_ent->inode);
                auto new_node = std::make_shared<VNode>(new_inode->type_and_perm & 0xFFF, new_inode->user_id, new_inode->group_id, new_inode->create_time, new_inode->access_time, new_inode->modify_time, new_inode->size, name_cp, inode_to_vnode_type(new_inode->type_and_perm & 0xF000), mi.filesystem);

                new_node->f_parent = root;
                new_node->fs_data = new Ext2VNodeDataStorage(new_inode);
                root->add_child(new_node);
                nodes.push_back(new_node);
            }
        }
    }

    return nodes;
}
