#pragma once

#include <errno.h>
#include <list.hpp>
#include <shared_pointer.hpp>
#include <types.h>

namespace kernel {
    class FileStream;
    class VNode;
    
    struct ResolvedMemoryEntry;
    struct ResolvableMemoryEntry;

    typedef std::SharedPtr<VNode> VNodePtr;

    struct FilesystemFlags {
        /**
         * @brief If the target file is a link the return value will be the node pointed to by this link
         * 
         */
        bool resolve_link : 1;

        /**
         * @brief Resolve links in the path, if this value is false will return ERR_LINK if a link is found
         * 
         */
        bool follow_links : 1;
    };

    class Filesystem {
    protected:
        Filesystem() { }

    public:
        virtual ~Filesystem() = default;

        virtual ValueOrError<void> umount();

        /**
         * @brief Get file
         *
         * This function is used in the path resolving in VFS.
         * It is only run if the filename does not exist in the children of root (it was not already resolved)
         *
         * @param root Where should the fs look for the file. Can be set to nullptr for the current location to be root of this filesystem
         * @param filename Filename to look for. This is guaranteed to be a filename and not a path.
         * @param flags Filesystem flags to change the behaviour of the resolving
         * @return ValueOrError<VNodePtr> VNodePtr for the filename or an error
         */
        virtual ValueOrError<VNodePtr> get_file(VNodePtr root, const char* filename, FilesystemFlags flags);

        /**
         * @brief Get files in directory
         *
         * This function is used by the VFS as the last step
         * of VFS::get_files function.
         *
         * @param root Where should the fs look for the files. Can be set to nullptr for the current location to be root of this filesystem
         * @param flags Filesystem flags to change the behaviour of the resolving
         * @return ValueOrError<std::List<VNodePtr>> List of nodes or an error
         */
        virtual ValueOrError<std::List<VNodePtr>> get_files(VNodePtr root, FilesystemFlags flags);

        virtual ValueOrError<VNodePtr> resolve_link(VNodePtr link, int depth);

        virtual ValueOrError<void> open(FileStream* stream, int mode);
        virtual ValueOrError<void> close(FileStream* stream);

        virtual ValueOrError<size_t> read(FileStream* stream, void* buffer, size_t length);
        virtual ValueOrError<size_t> write(FileStream* stream, const void* buffer, size_t length);

        virtual ValueOrError<size_t> seek(FileStream* stream, size_t position, int mode);

        virtual std::Optional<ResolvedMemoryEntry> resolve_mapping(const ResolvableMemoryEntry& mapping, virtaddr_t addr);
        virtual void sync_mapping(const ResolvedMemoryEntry& mapping);

        virtual ValueOrError<int> ioctl(FileStream* stream, u64_t request, void* arg);

        virtual ValueOrError<VNodePtr> symlink(VNodePtr root, const char* name, const char* dest);
        //virtual ValueOrError<VNodePtr> link(VNodePtr root, const char* name, VNodePtr dest);
        virtual ValueOrError<VNodePtr> mkdir(VNodePtr root, const char* name);
    };
}
