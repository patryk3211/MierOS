#include "fs_func.hpp"
#include "mount_info.hpp"
#include <unordered_map.hpp>
#include <fs/devicefs.hpp>
#include <assert.h>
#include "inode.hpp"
#include "data_storage.hpp"

using namespace kernel;

extern u16_t minor_num;
extern std::UnorderedMap<u16_t, MountInfo> mounted_filesystems;

ValueOrError<u16_t> mount(std::SharedPtr<VNode> fs_file) {
    MountInfo mi;

    DeviceFilesystem::instance()->block_read(fs_file, 2, 2, mi.superblock.ptr());
    if(mi.superblock->signature != 0xef53) return ERR_MOUNT_FAILED;
    mi.fs_file = fs_file;

    { // Read the superblock
        u32_t total_blocks = mi.superblock->total_blocks;
        u32_t blocks_per_group = mi.superblock->group_block_count;
        mi.block_group_count = (total_blocks / blocks_per_group) + (total_blocks % blocks_per_group == 0 ? 0 : 1);

        mi.block_size = 1024 << mi.superblock->blocks_size;

        mi.sb_ext = mi.superblock->version_major >= 1;
    }

    { // Read the block groups
        size_t byte_size = mi.block_group_count * sizeof(BlockGroup);
        size_t block_size = byte_size / mi.block_size + (byte_size % mi.block_size == 0 ? 0 : 1);
        mi.block_groups.resize(block_size * mi.block_size / sizeof(BlockGroup));

        DeviceFilesystem::instance()->block_read(fs_file, mi.get_lba(mi.get_group_descriptor_block(0)), mi.block_size / 512, mi.block_groups.ptr());
    }

    u16_t minor = minor_num++;
    mounted_filesystems.insert({ minor, mi });
    return minor;
}

void set_fs_object(u16_t minor, Filesystem* fs_obj) {
    auto mi = mounted_filesystems.at(minor);
    ASSERT_F(mi, "Invalid minor number provided");

    mi->filesystem = fs_obj;
    
    // Read the root inode
    INodePtr root_inode = read_inode(*mi, 2);
        
    // We must create the root node here since in the mount function we did not have the filesystem object.
    mi->root = std::make_shared<VNode>(root_inode->type_and_perm & 0xFFF, root_inode->user_id, root_inode->group_id, root_inode->create_time, root_inode->access_time, root_inode->modify_time, root_inode->size, "", VNode::DIRECTORY, mi->filesystem);
    mi->root->fs_data = new Ext2VNodeDataStorage(root_inode);
}
