#include "fs_func.hpp"
#include "mount_info.hpp"
#include <defines.h>
#include <fs/modulefs_functions.hpp>
#include <modules/module_header.h>
#include <tasking/thread.hpp>
#include <unordered_map.hpp>

using namespace kernel;

MODULE_HEADER static module_header header {
    .magic = MODULE_HEADER_MAGIC,
    .mod_name = "ext2",
    .dependencies = 0
};

FilesystemDriver filesystem_driver {
    .mount = &mount,

    .set_fs_object = &set_fs_object,

    .get_file = &get_file,
    .get_files = &get_files,

    .open = &open,
    .close = &close,

    .read = &read,

    .seek = &seek,

    .resolve_mapping = &resolve_mapping
};

u16_t mod_major;
u16_t minor_num;
std::UnorderedMap<u16_t, MountInfo> mounted_filesystems;

extern "C" int init() {
    mod_major = kernel::Thread::current()->f_current_module->major();
    minor_num = 0;
    return 0;
}

extern "C" int destroy() {
    return 0;
}
