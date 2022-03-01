#pragma once

#include <types.h>
#include <string.hpp>
#include <fs/filesystem.hpp>
#include <streams/filestream.hpp>
#include <atomic.hpp>

namespace kernel {
    #define FILE_OPEN_MODE_READ 0b01
    #define FILE_OPEN_MODE_WRITE 0b10
    #define FILE_OPEN_MODE_READ_WRITE 0b11

    class VNode {
    public:
        enum Type {
            DIRECTORY = 0,
            FILE = 1,
            DEVICE = 2,
            LINK = 3
        };

    private:
        u16_t f_permissions;
        u16_t f_user_id;
        u16_t f_group_id;

        time_t f_create_time;
        time_t f_access_time;
        time_t f_modify_time;

        u64_t f_size;

        std::String<> f_name;

        Filesystem* f_filesystem;

        Type f_type;

        std::Atomic<u32_t> f_ref_count;
    public:
        void* fs_data;

        VNode(u16_t permissions, u16_t user_id, u16_t group_id, time_t create_time, time_t access_time, time_t modify_time, u64_t size, const std::String<>& name, VNode::Type type, Filesystem* fs);
        ~VNode();

        FileStream* open(int mode);

        u64_t size() { return f_size; }
        Filesystem* filesystem() { return f_filesystem; }
        Type type() { return f_type; }

        void ref() { f_ref_count.fetch_add(1); }
        void unref() { f_ref_count.fetch_sub(1); }
        u32_t ref_count() { return f_ref_count.load(); }

        const std::String<>& name() const { return f_name; }
    private:
        void close(FileStream* stream);

        friend class FileStream;
    };
}
