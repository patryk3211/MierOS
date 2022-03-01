#include <fs/vnode.hpp>

using namespace kernel;

VNode::VNode(u16_t permissions, u16_t user_id, u16_t group_id, time_t create_time, time_t access_time, time_t modify_time, u64_t size, const std::String<>& name, VNode::Type type, Filesystem* fs) : 
    f_permissions(permissions), f_user_id(user_id), f_group_id(group_id), f_create_time(create_time), f_access_time(access_time), f_modify_time(modify_time), f_size(size), f_name(name), f_filesystem(fs), f_type(type) {

}
