#include <fs/vfs.hpp>

#include <cstddef.hpp>
#include <modules/module_manager.hpp>
#include <fs/modulefs.hpp>

using namespace kernel;

VFS* VFS::s_instance = 0;

VFS::VFS() 
    : f_rootNode(nullptr) {
    s_instance = this;
}

VFS::~VFS() {
}

ValueOrError<void> VFS::mount(VNodePtr fsFile, const char* fsType, const char* location) {
    auto mountHandler = f_filesystems.at(fsType);
    if(mountHandler) {
        auto result = (*mountHandler)(fsFile);
        if(!result)
            return result.errno();

        return mount(*result, location);
    } else {
        // The filesystem is not registered, let's try to load a module for it
        std::String<> modName = "FS-";
        modName += fsType;

        u16_t modMajor = ModuleManager::get().find_module(modName);
        if(modMajor == 0)
            return ENOTSUP;

        // TODO: [12.02.2023] This will be changed a bit, filesystem modules will be
        // calling the VFS::register_filesystem method to register a mount handler and
        // we will resolve all filesystems with the first part of this method. If the handler
        // wasn't found then we don't have the module loaded and we fail the mount entirely.
        auto* mod = ModuleManager::get().get_module(modMajor);
        auto result = ((FilesystemDriver*)mod->get_symbol_ptr("filesystem_driver"))->mount(fsFile);
        if(!result)
            return result.errno();

        auto* modFs = new ModuleFilesystem(modMajor, *result);
        return mount(modFs, location);
    }
}

ValueOrError<void> VFS::mount(Filesystem* fs, const char* location) {
    auto fsRootRes = fs->get_file(nullptr, "", {});
    if(!fsRootRes)
        return fsRootRes.errno();
    auto fsRoot = *fsRootRes;

    if(location[0] == 0 || (location[0] == '/' && location[1] == 0)) {
        // Mounting root
        if(f_rootNode) {
            // return ERR_MOUNT_FAILED;
        }

        f_rootNode = fsRoot;
    } else {
        // Mounting somewhere else
        auto locationNodeRes = get_file(nullptr, location, { .resolve_link = true, .follow_links = true });
        if(!locationNodeRes)
            return locationNodeRes.errno();
        auto locationNode = *locationNodeRes;

        fsRoot->mount(locationNode);
        fsRoot->f_parent->add_child(fsRoot);
    }

    return {};
}

ValueOrError<void> VFS::umount(const char* location) {
    /// TODO: [12.04.2022] And this
    panic("umount is unimplemented");
}

ValueOrError<VNodePtr> VFS::get_file(VNodePtr root, const char* path, FilesystemFlags flags, int depth) {
    if(!root) root = f_rootNode;

    const char* path_ptr = path;
    const char* next_separator = path;
    while(*next_separator != 0 && ((next_separator = strchr(path_ptr, '/')) != 0 || (next_separator = strchr(path_ptr, 0)))) {
        if(root->type() != VNode::DIRECTORY) return ENOTDIR;
        if(root->type() == VNode::LINK) {
            if(flags.follow_links) {
                auto linkDest = root->filesystem()->resolve_link(root, depth);
                if(!linkDest)
                    return linkDest.errno();
                else
                    root = *linkDest;
            } else
                return ELOOP;
        }

        size_t length = next_separator - path_ptr;
        if(length == 0) {
            path_ptr = next_separator + 1;
            continue;
        }

        char part[length + 1];
        memcpy(part, path_ptr, length);
        part[length] = 0;

        if(part[0] == 0 || (part[0] == '.' && part[1] == 0)) {
            path_ptr = next_separator + 1;
            continue;
        } else if(part[0] == '.' && part[1] == '.' && part[2] == 0) {
            if(root->f_parent) root = root->f_parent;
            path_ptr = next_separator + 1;
            continue;
        }

        auto next_root = root->f_children.at(part);
        if(!next_root) {
            // Call get_file with the current path part
            auto result = root->filesystem()->get_file(root, part, flags);
            if(!result)
                return result.errno();
            root = *result;
        } else {
            root = *next_root;
        }

        path_ptr = next_separator + 1;
    }

    // We have fully resolved the path, resolve the link if necessary
    if(root->type() == VNode::LINK) {
        if(flags.resolve_link) {
            auto linkDest = root->filesystem()->resolve_link(root, depth);
            if(!linkDest)
                return linkDest.errno();
            else
                return *linkDest;
        }
    }
    return root;
}

ValueOrError<std::List<VNodePtr>> VFS::get_files(VNodePtr root, const char* path, FilesystemFlags flags) {
    auto dir = get_file(root, path, flags);

    if(!dir) return dir.errno();
    VNodePtr ptr = *dir;

    return ptr->filesystem()->get_files(ptr, flags);
}

ValueOrError<std::Pair<VNodePtr, const char*>> VFS::resolve_path(VNodePtr root, const char* path, FilesystemFlags flags) {
    const char* filename = 0;

    char* lastSeparator = strlchr(path, '/');
    if(lastSeparator[1] == 0) {
        // Empty file name
        return EINVAL;
    }

    if(lastSeparator == 0) {
        filename = path;
    } else {
        filename = lastSeparator + 1;

        size_t length = strlen(path);
        char copy[length + 1];
        memcpy(copy, path, lastSeparator - path);
        copy[lastSeparator - path] = 0;

        auto res = get_file(root, copy, flags);
        if(!res)
            return res.errno();
        root = *res;
    }

    return std::Pair(root, filename);
}

void VFS::register_filesystem(const char* fsType, mount_handler_t* handler) {
    auto current = f_filesystems.at(fsType);
    if(!current) {
        f_filesystems.insert({ fsType, handler });
    }
}

void VFS::unregister_filesystem(const char* fsType) {
    f_filesystems.erase(fsType);
}
