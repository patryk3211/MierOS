#include <fs/devicefs.hpp>
#include <fs/vnode.hpp>

using namespace kernel;

DeviceFilesystem* DeviceFilesystem::instance = 0;

DeviceFilesystem::DeviceFilesystem() {
    root = new VNode();
}
