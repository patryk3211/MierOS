#pragma once

#include <fs/filesystem.hpp>
#include <fs/vnode.hpp>

namespace kernel {
    /**
     * @brief VNode only filesystem
     *
     * This is a simple helper class used by kernel filesystems (devfs, sysfs, etc.)
     * which implements basic filesystem functions like get file, mkdir, link. Since
     * the implementation of these functions would be the same for each fs (they are only
     * backed by vnodes), to not reimplement them every time we just inherit this class.
     */
    class VNodeFilesystem : public Filesystem {
    protected:
        VNodePtr f_root;

    public:
        VNodeFilesystem();
        ~VNodeFilesystem() = default;

        virtual ValueOrError<VNodePtr> get_file(VNodePtr root, const char* filename, FilesystemFlags flags) override;
        virtual ValueOrError<std::List<VNodePtr>> get_files(VNodePtr root, FilesystemFlags flags) override;

        virtual ValueOrError<VNodePtr> resolve_link(VNodePtr link, int depth) override;

        virtual ValueOrError<VNodePtr> symlink(VNodePtr root, const char* filename, const char* dest) override;
        virtual ValueOrError<VNodePtr> mkdir(VNodePtr root, const char* filename) override;

        /**
         * @brief Add link on path
         *
         * A helper function which resolves the path into a root and filename
         * pair that is later passed to the VNodeFilesystem::link method.
         *
         * @param path Path to put the link at
         * @param dest Node that the link should point to
         * @return Pointer to the link node or error
         */
        /* ValueOrError<VNodePtr> add_link(const char* path, VNodePtr dest);

        ValueOrError<VNodePtr> add_link(const char* path, const char* dest); */

        /**
         * @brief Resolve path into a root/filename pair
         *
         * This function resolves the given path into a root/filename path,
         * by default it will create any directory that doesn't exist on the
         * way to the filename.
         *
         * @param path Path to resolve
         * @param makeDirs Should directories which don't exist on the path be created
         * @return Pair of root/filename or error
         */
        ValueOrError<std::Pair<VNodePtr, const char*>> resolve_path(const char* path, VNodePtr root, bool makeDirs = true);
    
        /**
         * @brief Create directory at path
         *
         * A helper function which creates a directory node at the
         * given path.
         *
         * @param path Path to the directory
         * @return The directory node or error
         */
        ValueOrError<VNodePtr> add_directory(VNodePtr root, const char* path);
    };
}

