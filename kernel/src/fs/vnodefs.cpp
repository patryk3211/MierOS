#include <fs/vnodefs.hpp>
#include <fs/vfs.hpp>

using namespace kernel;

struct VNodeFs_LinkData : public VNodeDataStorage {
    std::String<> destination;

    VNodeFs_LinkData(const std::String<>& dest)
        : destination(dest) { }
    virtual ~VNodeFs_LinkData() = default;
};

VNodeFilesystem::VNodeFilesystem()
    : f_root(nullptr) {
    f_root = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, 0, "", VNode::DIRECTORY, this);
}

ValueOrError<VNodePtr> VNodeFilesystem::get_file(VNodePtr root, const char* filename, FilesystemFlags flags) {
    if(!root) root = f_root;
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");
    if(filename[0] == 0) return root;

    auto result = root->f_children.at(filename);
    if(!result) return ENOENT;

    auto file = *result;
    return file;
}

ValueOrError<std::List<VNodePtr>> VNodeFilesystem::get_files(VNodePtr root, FilesystemFlags flags) {
    if(!root) root = f_root;
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");

    std::List<VNodePtr> nodes;
    for(auto child : root->f_children)
        nodes.push_back(child.value);

    return nodes;
}

ValueOrError<VNodePtr> VNodeFilesystem::resolve_link(VNodePtr link, int depth) {
    ASSERT_F(link->filesystem() == this, "Using a VNode from a different filesystem");

    if(link->type() != VNode::LINK) return ENOLINK;
    if(depth >= 128) return ELOOP;
    return VFS::instance()->get_file(link->f_parent,
            static_cast<VNodeFs_LinkData*>(link->fs_data)->destination.c_str(),
            { .resolve_link = true, .follow_links = true }, depth + 1);
}

ValueOrError<VNodePtr> VNodeFilesystem::symlink(VNodePtr root, const char* filename, const char* dest) {
    if(!root) root = f_root;
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");

    auto node = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, 0, filename, VNode::LINK, this);
    node->fs_data = new VNodeFs_LinkData(dest);
    root->add_child(node);
    node->f_parent = root;

    return node;
}

ValueOrError<VNodePtr> VNodeFilesystem::mkdir(VNodePtr root, const char* filename) {
    if(get_file(root, filename, {}))
        return EEXIST;
    if(!root) root = f_root;
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");

    auto node = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, 0, filename, VNode::DIRECTORY, this);
    root->add_child(node);
    node->f_parent = root;

    return node;
}

ValueOrError<VNodePtr> VNodeFilesystem::add_directory(VNodePtr root, const char* path) {
    auto result = resolve_path(path, root);
    if(!result)
        return result.errno();

    return mkdir(result->key, result->value);
}

ValueOrError<std::Pair<VNodePtr, const char*>> VNodeFilesystem::resolve_path(const char* path, VNodePtr root, bool makeDirs) {
    if(!root) root = f_root;
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");

    const char* path_ptr = path;
    const char* next_separator;
    while((next_separator = strchr(path_ptr, '/')) != 0) {
        if(root->type() != VNode::DIRECTORY) return ENOTDIR;

        size_t length = next_separator - path_ptr;
        if(length == 0) {
            ++path_ptr;
            continue;
        }

        char part[length + 1];
        memcpy(part, path_ptr, length);
        part[length] = 0;

        auto next_root = get_file(root, part, { 1, 1 });
        if(next_root)
            root = *next_root;
        else {
            if(next_root.errno() == ENOENT && makeDirs) {
                auto result = mkdir(root, part);
                if(!result)
                    return result.errno();
                root = *result;
            } else {
                return next_root.errno();
            }
        }

        path_ptr = next_separator + 1;
    }
    
    return std::Pair(root, path_ptr);
}

