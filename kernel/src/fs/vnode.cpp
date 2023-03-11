#include <fs/vnode.hpp>

using namespace kernel;

VNode::VNode(u16_t permissions, u16_t user_id, u16_t group_id, time_t create_time, time_t access_time, time_t modify_time, u64_t size, const std::String<>& name, VNode::Type type, Filesystem* fs)
    : f_permissions(permissions)
    , f_user_id(user_id)
    , f_group_id(group_id)
    , f_create_time(create_time)
    , f_access_time(access_time)
    , f_modify_time(modify_time)
    , f_size(size)
    , f_parent(nullptr)
    , f_name(name)
    , f_filesystem(fs)
    , f_type(type) {
    fs_data = 0;
}

VNode::~VNode() {
    if(fs_data != 0) delete fs_data;
}

void VNode::add_child(VNodePtr child) {
    f_children.insert({ child->name(), child });
}

void VNode::mount(VNodePtr location) {
    f_parent = location->f_parent;
    f_parent->f_children.erase(location->f_name);
    f_name = location->f_name;
}

#define S_IFMT 0x0F000
#define S_IFBLK 0x06000
#define S_IFCHR 0x02000
#define S_IFIFO 0x01000
#define S_IFREG 0x08000
#define S_IFDIR 0x04000
#define S_IFLNK 0x0A000
#define S_IFSOCK 0x0C000

void VNode::stat(mieros_stat* statPtr) {
    statPtr->f_size = f_size;
    statPtr->f_blksize = 1;
    
    statPtr->f_blocks = f_size / 512;
    if(f_size % 512)
        statPtr->f_blocks += 1;

    statPtr->f_mode = f_permissions;
    statPtr->f_uid = f_user_id;
    statPtr->f_gid = f_group_id;

    statPtr->f_atime = f_access_time;
    statPtr->f_mtime = f_modify_time;
    statPtr->f_ctime = f_create_time;

    statPtr->f_links = 1;
    statPtr->f_ino = 0;
    statPtr->f_dev = 0;
    statPtr->f_rdev = 0;

    switch(f_type) {
        case VNode::FILE:
            statPtr->f_mode |= S_IFREG;
            break;
        case VNode::MOUNT:
        case VNode::DIRECTORY:
            statPtr->f_mode |= S_IFDIR;
            break;
        case VNode::BLOCK_DEVICE:
            statPtr->f_mode |= S_IFBLK;
            break;
        case VNode::CHARACTER_DEVICE:
            statPtr->f_mode |= S_IFCHR;
            break;
        case VNode::LINK:
            statPtr->f_mode |= S_IFLNK;
            break;
    }
}

