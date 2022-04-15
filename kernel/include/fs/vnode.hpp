#pragma once

#include <atomic.hpp>
#include <fs/filesystem.hpp>
#include <shared_pointer.hpp>
#include <streams/filestream.hpp>
#include <string.hpp>
#include <types.h>
#include <unordered_map.hpp>

#define FILE_OPEN_MODE_READ 0b01
#define FILE_OPEN_MODE_WRITE 0b10
#define FILE_OPEN_MODE_READ_WRITE 0b11

namespace kernel {
    struct VNodeDataStorage {
        virtual ~VNodeDataStorage() { }
    };

    class VNode {
    public:
        enum Type {
            DIRECTORY = 0,
            FILE = 1,
            DEVICE = 2,
            LINK = 3,
            MOUNT = 4
        };

        u16_t f_permissions;
        u16_t f_user_id;
        u16_t f_group_id;

        time_t f_create_time;
        time_t f_access_time;
        time_t f_modify_time;

        u64_t f_size;

        std::SharedPtr<VNode> f_parent;
        std::UnorderedMap<std::String<>, std::SharedPtr<VNode>> f_children;

        VNodeDataStorage* fs_data;

    private:
        std::String<> f_name;

        Filesystem* f_filesystem;

        Type f_type;

    public:
        VNode(u16_t permissions, u16_t user_id, u16_t group_id, time_t create_time, time_t access_time, time_t modify_time, u64_t size, const std::String<>& name, VNode::Type type, Filesystem* fs);
        ~VNode();

        u64_t size() { return f_size; }
        Filesystem* filesystem() { return f_filesystem; }
        Type type() { return f_type; }

        const std::String<>& name() const { return f_name; }
    };

    typedef std::SharedPtr<VNode> VNodePtr;
}
