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

DeviceFilesystem::DeviceFilesystem() {
    s_instance = this;
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

ValueOrError<int> DeviceFilesystem::ioctl(FileStream* stream, u64_t request, void* arg) {
    RUN_FUNC(stream->node(), ioctl, request, arg);
}

ValueOrError<VNodePtr> DeviceFilesystem::add_dev(const char* path, u16_t major, u16_t minor) {
    ASSERT_F(major != 0, "Cannot have a major number of 0");

    auto result = resolve_path(path);
    if(!result)
        return result.errno();

    if(result->key->f_children.at(result->value)) return EEXIST;

    auto node = std::make_shared<VNode>(0, 0, 0, 0, 0, 0, 0, result->value, VNode::DEVICE, this);
    node->fs_data = new DevFs_DevData(major, minor);
    result->key->add_child(node);
    node->f_parent = result->key;

    return node;
}

