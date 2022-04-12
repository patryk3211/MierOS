#include <fs/vfs.hpp>

#include <cstddef.hpp>

using namespace kernel;

VFS* VFS::s_instance = 0;

VFS::VFS() {
    s_instance = this;
}

VFS::~VFS() {
    
}

ValueOrError<void> VFS::mount(Filesystem* fs, const char* location) {
    if(!strcmp(location, "/")) {
        // Mounting root
        auto value = fs->get_file(nullptr, "/", { });

        if(value) f_rootNode = *value;
        else return value.errno();

        return { };
    }

    auto locationNode = get_file(nullptr, (std::String(location) + "/..").c_str(), { .resolve_link = true, .follow_links = true });

    /// TODO: [12.04.2022] Complete this
}

ValueOrError<void> VFS::umount(const char* location) {
    /// TODO: [12.04.2022] And this
}

ValueOrError<VNodePtr> VFS::get_file(VNodePtr root, const char* path, FilesystemFlags flags) {
    if(!root) root = f_rootNode;
    
    const char* path_ptr = path;
    const char* next_separator = path;
    while(*next_separator != 0 && ((next_separator = strchr(path_ptr, '/')) != 0 || (next_separator = strchr(path_ptr, 0)))) {
        if(root->type() != VNode::DIRECTORY) return ERR_NOT_A_DIRECTORY;
        if(root->type() == VNode::LINK) {
            if(flags.follow_links) {
                auto linkDest = root->filesystem()->resolve_link(root);
                if(!linkDest) return linkDest.errno();
                else root = *linkDest;
            } else return ERR_LINK;
        }

        size_t length = next_separator - path_ptr;
        if(length == 0) {
            path_ptr = next_separator + 1;
            continue;
        }

        char part[length+1];
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
        if(!next_root) break;

        root = *next_root;
        path_ptr = next_separator + 1;
    }

    if(*next_separator != 0) {
        // We have not finished resolving the path yet.
        return root->filesystem()->get_file(root, path_ptr, flags);
    } else {
        // We have fully resolved the path, resolve the link if necessary
        if(root->type() == VNode::LINK) {
            if(flags.resolve_link) {
                auto linkDest = root->filesystem()->resolve_link(root);
                if(!linkDest) return linkDest.errno();
                else return *linkDest;
            }
        }
        return root;
    }
}

ValueOrError<std::List<VNodePtr>> VFS::get_files(VNodePtr root, const char* path, FilesystemFlags flags) {
    auto dir = get_file(root, path, flags);

    if(!dir) return dir.errno();
    VNodePtr ptr = *dir;

    return ptr->filesystem()->get_files(ptr, "", flags);
}
