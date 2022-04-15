#pragma once

#include <fs/filesystem.hpp>
#include <fs/vnode.hpp>

namespace kernel {
    class VFS {
        static VFS* s_instance;

        VNodePtr f_rootNode;

    public:
        VFS();
        ~VFS();

        ValueOrError<void> mount(Filesystem* fs, const char* location);
        ValueOrError<void> umount(const char* location);

        ValueOrError<VNodePtr> get_file(VNodePtr root, const char* path, FilesystemFlags flags);
        ValueOrError<std::List<VNodePtr>> get_files(VNodePtr root, const char* path, FilesystemFlags flags);

        static VFS* instance() { return s_instance; }
    };
}
