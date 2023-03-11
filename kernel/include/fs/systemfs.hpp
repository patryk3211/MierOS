#pragma once

#include <fs/vnodefs.hpp>
#include <fs/vnode.hpp>

namespace kernel {
    struct SysFsVNodeData : public VNodeDataStorage {
        VNodePtr node;
        void* static_data;

        void* callback_arg;
        ValueOrError<size_t> (*read_callback)(void* cbArg, FileStream* stream, void* buffer, size_t length);
        ValueOrError<size_t> (*write_callback)(void* cbArg, FileStream* stream, const void* buffer, size_t length);

        // We can store 16 bytes of data for very small entries
        char small_data[16];

        SysFsVNodeData(const VNodePtr& node)
            : node(node) { }
    };

    struct SysFsStreamData {
        size_t offset;

        SysFsStreamData() {
            offset = 0;
        }
    };

    class SystemFilesystem : public VNodeFilesystem {
        static SystemFilesystem* s_instance;

        VNodePtr f_root;

    public:
        SystemFilesystem();
        virtual ~SystemFilesystem() = default;

        virtual ValueOrError<void> open(FileStream* stream, int mode);
        virtual ValueOrError<void> close(FileStream* stream);

        virtual ValueOrError<size_t> read(FileStream* stream, void* buffer, size_t length);
        virtual ValueOrError<size_t> write(FileStream* stream, const void* buffer, size_t length);

        virtual ValueOrError<size_t> seek(FileStream* stream, size_t position, int mode);

        virtual ValueOrError<void> stat(VNodePtr node, mieros_stat* stat);

        ValueOrError<SysFsVNodeData*> add_node(VNodePtr root, const char* path);

        static SystemFilesystem* instance();
    };
}

