#pragma once

#include <fs/filesystem.hpp>
#include <fs/vnode.hpp>

namespace kernel {
    typedef ValueOrError<Filesystem*> mount_handler_t(VNodePtr fsFile);

    class VFS {
        static VFS* s_instance;

        VNodePtr f_rootNode;

        std::UnorderedMap<std::String<>, mount_handler_t*> f_filesystems;

    public:
        VFS();
        ~VFS();

        void register_filesystem(const char* fsType, mount_handler_t* handler);
        void unregister_filesystem(const char* fsType);
        ValueOrError<void> mount(VNodePtr fsFile, const char* fsType, const char* location);

        ValueOrError<void> mount(Filesystem* fs, const char* location);
        ValueOrError<void> umount(const char* location);

        ValueOrError<VNodePtr> get_file(VNodePtr root, const char* path, FilesystemFlags flags, int depth = 0);
        ValueOrError<std::List<VNodePtr>> get_files(VNodePtr root, const char* path, FilesystemFlags flags);

        ValueOrError<std::Pair<VNodePtr, const char*>> resolve_path(VNodePtr root, const char* path, FilesystemFlags flags);

        static VFS* instance() { return s_instance; }

    private:
        std::Pair<std::String<>, VNodePtr> collect_mounts(VNodePtr node);
    };
}
