#include <assert.h>
#include <fs/devicefs.hpp>
#include <fs/vnode.hpp>
#include <modules/module_manager.hpp>
#include <unordered_map.hpp>

using namespace kernel;

struct DevFs_LinkData : public VNodeDataStorage {
    VNodePtr destination;

    DevFs_LinkData(const VNodePtr& dest)
        : destination(dest) { }
};

struct DevFs_DevData : public VNodeDataStorage {
    u16_t major;
    u16_t minor;

    DevFs_DevData(u16_t major, u16_t minor)
        : major(major)
        , minor(minor) { }
};

DeviceFilesystem* DeviceFilesystem::s_instance = 0;

DeviceFilesystem::DeviceFilesystem() 
    : root(nullptr) {
    root = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, 0, "", VNode::DIRECTORY, this);

    s_instance = this;
}

// We do need this function implemented because we are using it later on
ValueOrError<VNodePtr> DeviceFilesystem::get_file(VNodePtr root, const char* path, FilesystemFlags flags) {
    if(!root) root = this->root;
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");
    if(path[0] == 0) return root;

    auto result = root->f_children.at(path);
    if(!result) return ENOENT;

    auto file = *result;
    return (file->type() == VNode::LINK && flags.resolve_link) ? static_cast<DevFs_LinkData*>(file->fs_data)->destination : file;
}

ValueOrError<std::List<VNodePtr>> DeviceFilesystem::get_files(VNodePtr root, FilesystemFlags) {
    if(!root) root = this->root;
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");

    std::List<VNodePtr> nodes;
    for(auto child : root->f_children)
        nodes.push_back(child.value);

    return nodes;
}

ValueOrError<VNodePtr> DeviceFilesystem::resolve_link(VNodePtr link) {
    ASSERT_F(link->filesystem() == this, "Using a VNode from a different filesystem");

    if(link->type() != VNode::LINK) return ENOLINK;
    return static_cast<DevFs_LinkData*>(link->fs_data)->destination;
}

std::Pair<u16_t, DeviceFunctionTable*> DeviceFilesystem::get_function_table(VNodePtr node) {
    auto* numbers = static_cast<DevFs_DevData*>(node->fs_data);
    auto* mod = ModuleManager::get().get_module(numbers->major);
    if(!mod) return { 0, 0 };

    return { numbers->minor, (DeviceFunctionTable*)mod->get_symbol_ptr("device_function_table") };
}

#define RUN_FUNC(node, func, ...) {\
    auto result = get_function_table(node); \
    if(!result.key) \
        return ENODEV; \
    auto* f = result.value->func; \
    if(!f) \
        return ENOTSUP; \
    return f(result.key, __VA_ARGS__); \
}

ValueOrError<void> DeviceFilesystem::open(FileStream* stream, int mode) {
    RUN_FUNC(stream->node(), open, stream, mode);
}

ValueOrError<void> DeviceFilesystem::close(FileStream* stream) {
    RUN_FUNC(stream->node(), close, stream);
}

ValueOrError<size_t> DeviceFilesystem::read(FileStream* stream, void* buffer, size_t length) {
    RUN_FUNC(stream->node(), read, stream, buffer, length);
}

ValueOrError<size_t> DeviceFilesystem::write(FileStream* stream, const void* buffer, size_t length) {
    RUN_FUNC(stream->node(), write, stream, buffer, length);
}

ValueOrError<size_t> DeviceFilesystem::seek(FileStream* stream, size_t position, int mode) {
    RUN_FUNC(stream->node(), seek, stream, position, mode);
}

ValueOrError<u32_t> DeviceFilesystem::block_read(VNodePtr bdev, u64_t lba, u32_t sector_count, void* buffer) {
    RUN_FUNC(bdev, block_read, lba, sector_count, buffer);
}

ValueOrError<u32_t> DeviceFilesystem::block_write(VNodePtr bdev, u64_t lba, u32_t sector_count, const void* buffer) {
    RUN_FUNC(bdev, block_write, lba, sector_count, buffer);
}

ValueOrError<void> DeviceFilesystem::ioctl(FileStream* stream, u64_t request, void* arg) {
    RUN_FUNC(stream->node(), ioctl, request, arg);
}

ValueOrError<VNodePtr> DeviceFilesystem::add_dev(const char* path, u16_t major, u16_t minor) {
    ASSERT_F(major != 0, "Cannot have a major number of 0");

    VNodePtr root = this->root;

    const char* path_ptr = path;
    const char* next_separator;
    while((next_separator = strchr(path_ptr, '/')) != 0) {
        if(root->type() != VNode::DIRECTORY) return ENOTDIR;

        size_t length = next_separator - path_ptr;
        if(length == 0) {
            ++path_ptr;
            continue;
        }

        char part[length + 1];
        memcpy(part, path_ptr, length);
        part[length] = 0;

        auto next_root = get_file(root, part, { 1, 1 });
        if(next_root)
            root = *next_root;
        else {
            if(next_root.errno() == ENOENT) {
                auto root_new = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, 0, part, VNode::DIRECTORY, this);
                root->f_children.insert({ part, root_new });
                root = root_new;
            } else
                return next_root.errno();
        }

        path_ptr = next_separator + 1;
    }

    if(root->f_children.at(path_ptr)) return EEXIST;

    auto node = std::make_shared<VNode>(0, 0, 0, 0, 0, 0, 0, path_ptr, VNode::DEVICE, this);
    node->fs_data = new DevFs_DevData(major, minor);
    root->add_child(node);
    node->f_parent = root;

    return node;
}

ValueOrError<VNodePtr> DeviceFilesystem::add_link(const char* path, VNodePtr destination) {
    VNodePtr root = this->root;

    const char* path_ptr = path;
    const char* next_separator;
    while((next_separator = strchr(path_ptr, '/')) != 0) {
        if(root->type() != VNode::DIRECTORY) return ENOTDIR;

        size_t length = next_separator - path_ptr;
        if(length == 0) {
            ++path_ptr;
            continue;
        }

        char part[length + 1];
        memcpy(part, path_ptr, length);
        part[length] = 0;

        auto next_root = get_file(root, part, { 1, 1 });
        if(next_root)
            root = *next_root;
        else {
            if(next_root.errno() == ENOENT) {
                auto root_new = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, 0, part, VNode::DIRECTORY, this);
                root->f_children.insert({ part, root_new });
                root = root_new;
            } else
                return next_root.errno();
        }

        path_ptr = next_separator + 1;
    }

    if(root->f_children.at(path_ptr)) return EEXIST;

    auto node = std::make_shared<VNode>(0, 0, 0, 0, 0, 0, 0, path_ptr, VNode::LINK, this);
    node->fs_data = new DevFs_LinkData(destination);
    root->add_child(node);
    node->f_parent = root;

    return node;
}
