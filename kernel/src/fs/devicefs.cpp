#include <fs/devicefs.hpp>
#include <fs/vnode.hpp>
#include <assert.h>
#include <unordered_map.hpp>
#include <modules/module_manager.hpp>

using namespace kernel;

struct DevFs_Directory_VData {
    std::UnorderedMap<std::String<>, VNode*> children;
};

DeviceFilesystem* DeviceFilesystem::s_instance = 0;

DeviceFilesystem::DeviceFilesystem() {
    root = new VNode(0777, 0, 0, 0, 0, 0, 0, "", VNode::DIRECTORY);

    s_instance = this;
}

ValueOrError<VNode*> DeviceFilesystem::get_file(VNode* root, const char* path, FilesystemFlags flags) {
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");
    if(root == 0) root = this->root;

    const char* path_ptr = path;
    const char* next_separator;
    while(*path_ptr != 0 && ((next_separator = strchr(path, '/')) != 0 || (next_separator = strchr(path, 0)))) {
        if(root->type() != VNode::DIRECTORY) return ERR_NOT_A_DIRECTORY;

        size_t length = next_separator - path_ptr;
        if(length == 0) continue;

        char part[length+1];
        memcpy(part, path_ptr, length);
        part[length] = 0;

        auto next_root = ((DevFs_Directory_VData*)root->fs_data)->children.at(part);
        if(!next_root) return ERR_FILE_NOT_FOUND;
        root = *next_root;
        if(root->type() == VNode::LINK && *next_separator != 0) {
            if(flags.follow_links) root = (VNode*)root->fs_data;
            else return ERR_LINK;
        }

        path_ptr = next_separator;
    }
    return (root->type() == VNode::LINK && flags.resolve_link) ? (VNode*)root->fs_data : root;
}

ValueOrError<void> DeviceFilesystem::open(FileStream* stream, int mode) {
    u32_t number = (u32_t)stream->node()->fs_data;
    auto* func = get_module_symbol<DevFsFunctionTable>(number >> 16, "dev_func_tab")->open;
    if(func != 0) return func(number & 0xFFFF, stream, mode);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<void> DeviceFilesystem::close(FileStream* stream) {
    u32_t number = (u32_t)stream->node()->fs_data;
    auto* func = get_module_symbol<DevFsFunctionTable>(number >> 16, "dev_func_tab")->close;
    if(func != 0) return func(number & 0xFFFF, stream);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<size_t> DeviceFilesystem::read(FileStream* stream, void* buffer, size_t length) {
    u32_t number = (u32_t)stream->node()->fs_data;
    auto* func = get_module_symbol<DevFsFunctionTable>(number >> 16, "dev_func_tab")->read;
    if(func != 0) return func(number & 0xFFFF, stream, buffer, length);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<size_t> DeviceFilesystem::write(FileStream* stream, const void* buffer, size_t length) {
    u32_t number = (u32_t)stream->node()->fs_data;
    auto* func = get_module_symbol<DevFsFunctionTable>(number >> 16, "dev_func_tab")->write;
    if(func != 0) return func(number & 0xFFFF, stream, buffer, length);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<size_t> DeviceFilesystem::seek(FileStream* stream, size_t position, int mode) {
    u32_t number = (u32_t)stream->node()->fs_data;
    auto* func = get_module_symbol<DevFsFunctionTable>(number >> 16, "dev_func_tab")->seek;
    if(func != 0) return func(number & 0xFFFF, stream, position, mode);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<VNode*> DeviceFilesystem::add_dev(const char* path, u16_t major, u16_t minor) {
    ASSERT_F(major != 0, "Cannot have a major number of 0");

    VNode* root = this->root;

    const char* path_ptr = path;
    const char* next_separator;
    while((next_separator = strchr(path_ptr, '/')) != 0) {
        if(root->type() != VNode::DIRECTORY) return ERR_NOT_A_DIRECTORY;

        size_t length = next_separator - path_ptr;
        if(length == 0) continue;

        char part[length+1];
        memcpy(part, path_ptr, length);
        part[length] = 0;

        auto next_root = get_file(root, part, { 1, 1 });
        if(next_root) root = *next_root;
        else {
            if(next_root.errno() == ERR_FILE_NOT_FOUND) root = new VNode(0777, 0, 0, 0, 0, 0, 0, part, VNode::DIRECTORY);
            else return next_root.errno();
        }

        path_ptr = next_separator;
    }

    if(((DevFs_Directory_VData*)root->fs_data)->children.at(path_ptr)) return ERR_FILE_EXISTS;

    VNode* node = new VNode(0, 0, 0, 0, 0, 0, 0, path_ptr, VNode::DEVICE);
    node->fs_data = (void*)((major << 16) | minor);
    ((DevFs_Directory_VData*)root->fs_data)->children.insert({ path_ptr, node });

    return node;
}