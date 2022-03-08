#include "fs_func.hpp"
#include "mount_info.hpp"
#include <unordered_map.hpp>
#include <fs/devicefs.hpp>
#include <assert.h>

using namespace kernel;

extern u16_t minor_num;
extern std::UnorderedMap<u16_t, MountInfo> mounted_filesystems;

ValueOrError<u16_t> mount(VNode* fs_file) {
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
}

void set_fs_object(u16_t minor, Filesystem* fs_obj) {
    auto val = mounted_filesystems.at(minor);
    ASSERT_F(val, "Invalid minor number provided");

    val->filesystem = fs_obj;
}
