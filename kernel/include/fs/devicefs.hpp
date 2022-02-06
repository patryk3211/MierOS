#pragma once

#include <fs/filesystem.hpp>

namespace kernel {
    class DeviceFilesystem : public Filesystem {
        static DeviceFilesystem* instance;

        VNode* root;
    public:
        DeviceFilesystem();
        ~DeviceFilesystem() { }
    };
}
