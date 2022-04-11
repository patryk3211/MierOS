#include <fs/vfs.hpp>

#include <cstddef.hpp>

using namespace kernel;

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
}

ValueOrError<void> VFS::umount(const char* location) {

}

ValueOrError<VNodePtr> VFS::get_file(VNodePtr root, const char* path, FilesystemFlags flags) {
    if(!root) root = f_rootNode;
    
}

ValueOrError<std::List<VNodePtr>> VFS::get_files(VNodePtr root, const char* path, FilesystemFlags flags) {

}
