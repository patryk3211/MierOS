#include "fs_func.hpp"
#include "mount_info.hpp"
#include <unordered_map.hpp>
#include <fs/devicefs.hpp>
#include <assert.h>

using namespace kernel;

extern u16_t minor_num;
extern std::UnorderedMap<u16_t, MountInfo> mounted_filesystems;

ValueOrError<u16_t> mount(std::SharedPtr<VNode> fs_file) {
    MountInfo mi;

    DeviceFilesystem::instance()->block_read(fs_file, 2, 2, mi.superblock.ptr());
    if(mi.superblock->signature != 0xef53) return ERR_MOUNT_FAILED;
    mi.fs_file = fs_file;

    {
        u32_t total_blocks = mi.superblock->total_blocks;
        u32_t blocks_per_group = mi.superblock->group_block_count;
        mi.block_group_count = (total_blocks / blocks_per_group) + (total_blocks % blocks_per_group == 0 ? 0 : 1);

        mi.block_size = 1024 << mi.superblock->blocks_size;

        mi.sb_ext = mi.superblock->version_major >= 1;
    }

    {
        size_t byte_size = mi.block_group_count * sizeof(BlockGroup);
        size_t block_size = byte_size / mi.block_size + (byte_size % mi.block_size == 0 ? 0 : 1);
        mi.block_groups.resize(block_size * mi.block_size / sizeof(BlockGroup));

        DeviceFilesystem::instance()->block_read(fs_file, mi.get_lba(mi.get_group_descriptor_block(0)), mi.block_size / 512, mi.block_groups.ptr());
    }

    {

    }

    u16_t minor = minor_num++;
    mounted_filesystems.insert({ minor, mi });
    return minor;
}

void set_fs_object(u16_t minor, Filesystem* fs_obj) {
    auto val = mounted_filesystems.at(minor);
    ASSERT_F(val, "Invalid minor number provided");

    val->filesystem = fs_obj;
}
