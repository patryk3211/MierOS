#include <fs/vnodefs.hpp>

using namespace kernel;

struct VNodeFs_LinkData : public VNodeDataStorage {
    VNodePtr destination;

    VNodeFs_LinkData(const VNodePtr& dest)
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
    return (file->type() == VNode::LINK && flags.resolve_link) ? static_cast<VNodeFs_LinkData*>(file->fs_data)->destination : file;
}

ValueOrError<std::List<VNodePtr>> VNodeFilesystem::get_files(VNodePtr root, FilesystemFlags flags) {
    if(!root) root = f_root;
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");

    std::List<VNodePtr> nodes;
    for(auto child : root->f_children)
        nodes.push_back(child.value);

    return nodes;
}

ValueOrError<VNodePtr> VNodeFilesystem::resolve_link(VNodePtr link) {
    ASSERT_F(link->filesystem() == this, "Using a VNode from a different filesystem");

    if(link->type() != VNode::LINK) return ENOLINK;
    return static_cast<VNodeFs_LinkData*>(link->fs_data)->destination;
}

ValueOrError<VNodePtr> VNodeFilesystem::link(VNodePtr root, const char* filename, VNodePtr dest, bool symbolic) {
    if(!symbolic)
        return ENOTSUP;
    if(get_file(root, filename, {}))
        return EEXIST;

    auto node = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, 0, filename, VNode::LINK, this);
    node->fs_data = new VNodeFs_LinkData(dest);
    root->add_child(node);
    node->f_parent = root;

    return node;
}

ValueOrError<VNodePtr> VNodeFilesystem::mkdir(VNodePtr root, const char* filename) {
    if(get_file(root, filename, {}))
        return EEXIST;

    auto node = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, 0, filename, VNode::DIRECTORY, this);
    root->add_child(node);
    node->f_parent = root;

    return node;
}

ValueOrError<VNodePtr> VNodeFilesystem::add_link(const char* path, VNodePtr dest) {
    auto result = resolve_path(path);
    if(!result)
        return result.errno();

    return link(result->key, result->value, dest, true);
}

ValueOrError<std::Pair<VNodePtr, const char*>> VNodeFilesystem::resolve_path(const char* path, bool makeDirs) {
    VNodePtr root = f_root;

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
            if(next_root.errno() == ENOENT && makeDirs)
                root = *mkdir(root, path);
            else
                return next_root.errno();
        }

        path_ptr = next_separator + 1;
    }
    
    return std::Pair(root, path_ptr);
}

