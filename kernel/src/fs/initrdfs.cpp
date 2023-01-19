#include <fs/initrdfs.hpp>

#include <fs/vnode.hpp>

using namespace kernel;

struct TarHeader {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];

    size_t getSize() {
        size_t val = 0;
        u32_t mul = 1;

        for(int i = 10; i >= 0; i--) {
            val += (size[i] - '0') * mul;
            mul *= 8;
        }

        return val;
    }
};

int oct2bin(char* str, int size) {
    int n = 0;
    char* c = str;
    while(size-- > 0) {
        n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}

struct InitRdVNodeData : VNodeDataStorage {
    TarHeader* ptr;

    InitRdVNodeData(TarHeader* ptr)
        : ptr(ptr) { }

    virtual ~InitRdVNodeData() = default;
};

InitRdFilesystem::InitRdFilesystem(void* initrd)
    : f_memory(initrd) {
    f_root = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, 0, "", VNode::DIRECTORY, this);
    f_root->fs_data = new InitRdVNodeData((TarHeader*)initrd);
}

VNode::Type get_vnode_type(char typeFlag) {
    switch(typeFlag) {
        case 0:
        case '0':
            return VNode::FILE;
        case '2':
            return VNode::LINK;
        case '3':
        case '4':
            return VNode::DEVICE;
        case '5':
            return VNode::DIRECTORY;
    }

    // Interpret all other types as a regular file
    return VNode::FILE;
}

ValueOrError<VNodePtr> InitRdFilesystem::get_node(VNodePtr root, const char* file) {
    if(root->type() != VNode::DIRECTORY) return ERR_NOT_A_DIRECTORY;

    if(file[0] == 0 || !strcmp(file, "."))
        return root;

    if(!strcmp(file, "..")) {
        if(root->f_parent)
            return root->f_parent;
        else
            return root;
    }

    auto next_root = root->f_children.at(file);
    if(next_root) {
        return *next_root;
    } else {
        // Read from disk
        auto* node_data = static_cast<InitRdVNodeData*>(root->fs_data);

        TarHeader* header = node_data->ptr;
        while(memcmp((char*)header + 257, "ustar", 5)) {
            int fileSize = oct2bin(header->size, 11);
            if(!strcmp(header->filename, file)) {
                // We found a file
                VNode::Type vtype = get_vnode_type(header->typeflag[0]);

                auto vnode = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, fileSize, header->filename, vtype, this);
                vnode->f_parent = root;
                vnode->fs_data = new InitRdVNodeData((TarHeader*)((u8_t*)header + 512));
                root->f_children.insert({ vnode->name(), vnode });

                return vnode;
            }
            header = (TarHeader*)((virtaddr_t)header + 512 + ((fileSize / 512 + (fileSize % 512 == 0 ? 0 : 1)) * 512));
        }

        return ERR_FILE_NOT_FOUND;
    }
}

ValueOrError<VNodePtr> InitRdFilesystem::get_file(VNodePtr root, const char* path, FilesystemFlags flags) {
    if(!root) root = f_root;
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");

    const char* path_ptr = path;
    const char* next_separator;
    while((next_separator = strchr(path_ptr, '/')) != 0) {
        size_t length = next_separator - path_ptr;
        if(length == 0) {
            ++path_ptr;
            continue;
        }

        char part[length + 1];
        memcpy(part, path_ptr, length);
        part[length] = 0;

        auto stat = get_node(root, part);
        if(!stat) return stat.errno();

        root = *stat;

        path_ptr = next_separator + 1;
    }

    return get_node(root, path_ptr);
}

ValueOrError<std::List<VNodePtr>> InitRdFilesystem::get_files(VNodePtr root, const char* path, FilesystemFlags flags) {
    if(!root) root = f_root;
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");

    auto val = get_file(root, path, flags);
    if(val.errno() != 0) return val.errno();

    auto node = *val;
    if(node->type() == VNode::LINK) {
        if(flags.follow_links)
            return ERR_UNIMPLEMENTED; /// TODO: [19.01.2023] Handle symbolic links
        else
            return ERR_LINK;
    }

    std::List<std::SharedPtr<VNode>> nodes;
    for(auto val : node->f_children)
        nodes.push_back(val.value);

    // Read from disk
    auto* node_data = static_cast<InitRdVNodeData*>(node->fs_data);
    TarHeader* header = node_data->ptr;
    while(memcmp((char*)header + 257, "ustar", 5)) {
        int fileSize = oct2bin(header->size, 11);
        if(node->f_children.find(header->filename) == node->f_children.end()) {
            // This node was not loaded before so we can load it now
            auto childNode = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, fileSize, header->filename, get_vnode_type(header->typeflag[0]), this);
            childNode->f_parent = node;
            childNode->fs_data = new InitRdVNodeData((TarHeader*)((u8_t*)header + 512));
            node->f_children.insert({ childNode->name(), childNode });
            nodes.push_back(childNode);
        }
        header = (TarHeader*)((virtaddr_t)header + 512 + ((fileSize / 512 + (fileSize % 512 == 0 ? 0 : 1)) * 512));
    }

    return nodes;
}

ValueOrError<void> InitRdFilesystem::open(FileStream* stream, int mode) {

}

ValueOrError<void> InitRdFilesystem::close(FileStream* stream) {

}

ValueOrError<size_t> InitRdFilesystem::read(FileStream* stream, void* buffer, size_t length) {

}

ValueOrError<size_t> InitRdFilesystem::write(FileStream* stream, const void* buffer, size_t length) {

}

ValueOrError<size_t> InitRdFilesystem::seek(FileStream* stream, size_t position, int mode) {

}
