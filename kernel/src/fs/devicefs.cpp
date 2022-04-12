#include <fs/devicefs.hpp>
#include <fs/vnode.hpp>
#include <assert.h>
#include <unordered_map.hpp>
#include <modules/module_manager.hpp>

using namespace kernel;

struct DevFs_LinkData : public VNodeDataStorage {
    std::SharedPtr<VNode> destination;

    DevFs_LinkData(const std::SharedPtr<VNode>& dest) : destination(dest) { }
};

struct DevFs_DevData : public VNodeDataStorage {
    u16_t major;
    u16_t minor;

    DevFs_DevData(u16_t major, u16_t minor) : major(major), minor(minor) { }
};

DeviceFilesystem* DeviceFilesystem::s_instance = 0;

DeviceFilesystem::DeviceFilesystem() {
    root = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, 0, "", VNode::DIRECTORY, this);

    s_instance = this;
}

ValueOrError<std::SharedPtr<VNode>> DeviceFilesystem::get_file(std::SharedPtr<VNode> root, const char* path, FilesystemFlags flags) {
    if(!root) root = this->root;
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");

    const char* path_ptr = path;
    const char* next_separator = path;
    while(*next_separator != 0 && ((next_separator = strchr(path_ptr, '/')) != 0 || (next_separator = strchr(path_ptr, 0)))) {
        if(root->type() != VNode::DIRECTORY) return ERR_NOT_A_DIRECTORY;

        size_t length = next_separator - path_ptr;
        if(length == 0) {
            path_ptr = next_separator + 1;
            continue;
        }

        char part[length+1];
        memcpy(part, path_ptr, length);
        part[length] = 0;

        if(part[0] == 0 || (part[0] == '.' && part[1] == 0)) {
            path_ptr = next_separator + 1;
            continue;
        } else if(part[0] == '.' && part[1] == '.' && part[2] == 0) {
            if(root->f_parent) root = root->f_parent;
            path_ptr = next_separator + 1;
            continue;
        }

        auto next_root = root->f_children.at(part);
        if(!next_root) return ERR_FILE_NOT_FOUND;
        root = *next_root;
        if(root->type() == VNode::LINK && *next_separator != 0) {
            if(flags.follow_links) root = static_cast<DevFs_LinkData*>(root->fs_data)->destination;
            else return ERR_LINK;
        }

        path_ptr = next_separator+1;
    }
    return (root->type() == VNode::LINK && flags.resolve_link) ? static_cast<DevFs_LinkData*>(root->fs_data)->destination : root;
}

ValueOrError<std::List<std::SharedPtr<VNode>>> DeviceFilesystem::get_files(std::SharedPtr<VNode> root, const char* path, FilesystemFlags flags) {
    if(!root) root = this->root;
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");

    const char* path_ptr = path;
    const char* next_separator = path;
    while(*next_separator != 0 && ((next_separator = strchr(path_ptr, '/')) != 0 || (next_separator = strchr(path_ptr, 0)))) {
        if(root->type() != VNode::DIRECTORY) return ERR_NOT_A_DIRECTORY;

        size_t length = next_separator - path_ptr;
        if(length == 0) continue;

        char part[length+1];
        memcpy(part, path_ptr, length);
        part[length] = 0;

        auto next_root = root->f_children.at(part);
        if(!next_root) return ERR_FILE_NOT_FOUND;
        root = *next_root;
        if(root->type() == VNode::LINK) {
            if(flags.follow_links) root = static_cast<DevFs_LinkData*>(root->fs_data)->destination;
            else return ERR_LINK;
        }

        path_ptr = next_separator+1;
    }
    if(root->type() != VNode::DIRECTORY) return ERR_NOT_A_DIRECTORY;
    
    std::List<std::SharedPtr<VNode>> nodes;

    for(auto child : root->f_children)
        nodes.push_back(child.value);
    
    return nodes;
}

ValueOrError<VNodePtr> DeviceFilesystem::resolve_link(VNodePtr link) {
    ASSERT_F(link->filesystem() == this, "Using a VNode from a different filesystem");

    if(link->type() != VNode::LINK) return ERR_NOT_LINK;
    return root = static_cast<DevFs_LinkData*>(root->fs_data)->destination;
}

ValueOrError<void> DeviceFilesystem::open(FileStream* stream, int mode) {
    auto* numbers = static_cast<DevFs_DevData*>(stream->node()->fs_data);
    auto* func = get_module_symbol<DevFsFunctionTable>(numbers->major, "dev_func_tab")->open;
    if(func != 0) return func(numbers->minor, stream, mode);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<void> DeviceFilesystem::close(FileStream* stream) {
    auto* numbers = static_cast<DevFs_DevData*>(stream->node()->fs_data);
    auto* func = get_module_symbol<DevFsFunctionTable>(numbers->major, "dev_func_tab")->close;
    if(func != 0) return func(numbers->minor, stream);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<size_t> DeviceFilesystem::read(FileStream* stream, void* buffer, size_t length) {
    auto* numbers = static_cast<DevFs_DevData*>(stream->node()->fs_data);
    auto* func = get_module_symbol<DevFsFunctionTable>(numbers->major, "dev_func_tab")->read;
    if(func != 0) return func(numbers->minor, stream, buffer, length);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<size_t> DeviceFilesystem::write(FileStream* stream, const void* buffer, size_t length) {
    auto* numbers = static_cast<DevFs_DevData*>(stream->node()->fs_data);
    auto* func = get_module_symbol<DevFsFunctionTable>(numbers->major, "dev_func_tab")->write;
    if(func != 0) return func(numbers->minor, stream, buffer, length);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<size_t> DeviceFilesystem::seek(FileStream* stream, size_t position, int mode) {
    auto* numbers = static_cast<DevFs_DevData*>(stream->node()->fs_data);
    auto* func = get_module_symbol<DevFsFunctionTable>(numbers->major, "dev_func_tab")->seek;
    if(func != 0) return func(numbers->minor, stream, position, mode);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<u32_t> DeviceFilesystem::block_read(std::SharedPtr<VNode> bdev, u64_t lba, u32_t sector_count, void* buffer) {
    auto* numbers = static_cast<DevFs_DevData*>(bdev->fs_data);
    auto* func = get_module_symbol<DevFsFunctionTable>(numbers->major, "dev_func_tab")->block_read;
    if(func != 0) return func(numbers->minor, lba, sector_count, buffer);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<u32_t> DeviceFilesystem::block_write(std::SharedPtr<VNode> bdev, u64_t lba, u32_t sector_count, const void* buffer) {
    auto* numbers = static_cast<DevFs_DevData*>(bdev->fs_data);
    auto* func = get_module_symbol<DevFsFunctionTable>(numbers->major, "dev_func_tab")->block_write;
    if(func != 0) return func(numbers->minor, lba, sector_count, buffer);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<std::SharedPtr<VNode>> DeviceFilesystem::add_dev(const char* path, u16_t major, u16_t minor) {
    ASSERT_F(major != 0, "Cannot have a major number of 0");

    std::SharedPtr<VNode> root = this->root;

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

        auto next_root = get_file(root, part, { 1, 1 });
        if(next_root) root = *next_root;
        else {
            if(next_root.errno() == ERR_FILE_NOT_FOUND) {
                auto root_new = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, 0, part, VNode::DIRECTORY, this);
                root->f_children.insert({ part, root_new });
                root = root_new;
            } else return next_root.errno();
        }

        path_ptr = next_separator+1;
    }

    if(root->f_children.at(path_ptr)) return ERR_FILE_EXISTS;

    auto node = std::make_shared<VNode>(0, 0, 0, 0, 0, 0, 0, path_ptr, VNode::DEVICE, this);
    node->fs_data = new DevFs_DevData(major, minor);
    root->f_children.insert({ path_ptr, node });

    return node;
}

ValueOrError<std::SharedPtr<VNode>> DeviceFilesystem::add_link(const char* path, std::SharedPtr<VNode> destination) {
    std::SharedPtr<VNode> root = this->root;

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

        auto next_root = get_file(root, part, { 1, 1 });
        if(next_root) root = *next_root;
        else {
            if(next_root.errno() == ERR_FILE_NOT_FOUND) {
                auto root_new = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, 0, part, VNode::DIRECTORY, this);
                root->f_children.insert({ part, root_new });
                root = root_new;
            } else return next_root.errno();
        }

        path_ptr = next_separator+1;
    }

    if(root->f_children.at(path_ptr)) return ERR_FILE_EXISTS;

    auto node = std::make_shared<VNode>(0, 0, 0, 0, 0, 0, 0, path_ptr, VNode::LINK, this);
    node->fs_data = new DevFs_LinkData(destination);
    root->f_children.insert({ path_ptr, node });

    return node;
}
