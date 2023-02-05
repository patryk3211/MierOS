#include <fs/initrdfs.hpp>
#include <memory/page/filepage.hpp>

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
} PACKED;

struct InitRdVNodeData : VNodeDataStorage {
    TarHeader* ptr;

    InitRdVNodeData(TarHeader* ptr)
        : ptr(ptr) { }

    virtual ~InitRdVNodeData() = default;
};

void InitRdFilesystem::parse_header(void* headerPtr) {
    TarHeader* header = (TarHeader*)headerPtr;

    VNodePtr root = f_root;

    const char* path_ptr = header->filename;
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
        if(!stat) {
            // We need to create a directory node here
            VNodePtr node = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, header->getSize(), part, VNode::DIRECTORY, this);
            node->f_parent = root;
            root->add_child(node);
            node->fs_data = 0;
        } else {
            root = *stat;
        }

        path_ptr = next_separator + 1;
    }

    if(strlen(path_ptr) > 0) {
        // There is still something left to process, create a node
        // with type set to the type in tar header
        VNode::Type vtype = get_vnode_type(header->typeflag[0]);
        VNodePtr node = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, header->getSize(), path_ptr, vtype, this);
        if(vtype == VNode::FILE) {
            // We only need the fs_data to be set for regular files
            node->fs_data = new InitRdVNodeData(header);
        }
        node->f_parent = root;
        root->add_child(node);
    }
}

InitRdFilesystem::InitRdFilesystem(void* initrd) {
    f_root = std::make_shared<VNode>(0777, 0, 0, 0, 0, 0, 0, "", VNode::DIRECTORY, this);
    f_root->fs_data = 0;

    // Convert the tar archive into our VFS structure
    TarHeader* header = (TarHeader*)initrd;
    while(memcmp((char*)header + 257, "ustar", 5)) {
        int fileSize = header->getSize();
        parse_header(header);
        header = (TarHeader*)((virtaddr_t)header + 512 + ((fileSize / 512 + (fileSize % 512 == 0 ? 0 : 1)) * 512));
    }
}

VNode::Type InitRdFilesystem::get_vnode_type(char typeFlag) {
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
    if(next_root)
        return *next_root;
    else
        return ERR_FILE_NOT_FOUND;
}

ValueOrError<VNodePtr> InitRdFilesystem::get_file(VNodePtr root, const char* filename, FilesystemFlags) {
    if(!root) root = f_root;
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");
    if(filename[0] == 0) return root;

    /// This will probably never be called, should consider just returning file not found error

    return get_node(root, filename);
}

ValueOrError<std::List<VNodePtr>> InitRdFilesystem::get_files(VNodePtr root, FilesystemFlags) {
    if(!root) root = f_root;
    ASSERT_F(root->filesystem() == this, "Using a VNode from a different filesystem");

    std::List<VNodePtr> nodes;
    for(auto val : root->f_children)
        nodes.push_back(val.value);

    return nodes;
}

struct InitRdStreamData {
    size_t offset;

    InitRdStreamData() {
        offset = 0;
    }
};

ValueOrError<void> InitRdFilesystem::open(FileStream* stream, int) {
    stream->fs_data = new InitRdStreamData();
    return { };
}

ValueOrError<void> InitRdFilesystem::close(FileStream* stream) {
    delete reinterpret_cast<InitRdStreamData*>(stream->fs_data);
    return { };
}

ValueOrError<size_t> InitRdFilesystem::read(FileStream* stream, void* buffer, size_t length) {
    auto* stream_data = reinterpret_cast<InitRdStreamData*>(stream->fs_data);
    size_t max_size = stream->node()->f_size;
    if(stream_data->offset >= max_size) return (size_t)0;

    size_t length_left = max_size - stream_data->offset;
    if(length > length_left) length = length_left;

    auto* vnode_data = reinterpret_cast<InitRdVNodeData*>(stream->node()->fs_data);
    memcpy(buffer, (u8_t*)vnode_data->ptr + 512 + stream_data->offset, length);

    stream_data->offset += length;
    return length;
}

ValueOrError<size_t> InitRdFilesystem::write(FileStream*, const void*, size_t) {
    /// TODO: [20.01.2023] This should be changed to 'Filesystem Read-only Error'
    return ERR_UNIMPLEMENTED;
}

ValueOrError<size_t> InitRdFilesystem::seek(FileStream* stream, size_t position, int mode) {
    auto* stream_data = reinterpret_cast<InitRdStreamData*>(stream->fs_data);
    size_t old_offset = stream_data->offset;
    switch(mode) {
        case SEEK_MODE_CUR:
            stream_data->offset += position;
            break;
        case SEEK_MODE_BEG:
            stream_data->offset = position;
            break;
        case SEEK_MODE_END:
            stream_data->offset = stream->node()->f_size + position;
            break;
    }
    return old_offset;
}

ValueOrError<void> InitRdFilesystem::umount() {
    return { };
}

PhysicalPage InitRdFilesystem::resolve_mapping(const FilePage& mapping, virtaddr_t addr) {
    size_t offset = ((addr - mapping.start_addr()) & ~0xFFF) + mapping.offset();

    auto vnode = mapping.file();

    auto pageOpt = vnode->f_shared_pages.at(offset);
    if(pageOpt)
        return *pageOpt;


    auto* vnode_data = reinterpret_cast<InitRdVNodeData*>(vnode->fs_data);
    if(!vnode_data)
        return nullptr;

    PhysicalPage page;
    page.flags() = PageFlags(true, true, false, false, true, false);

    auto& pager = Pager::active();
    pager.lock();
    virtaddr_t ptr = pager.kmap(page.addr(), 1, page.flags());

    ssize_t lengthLeft = vnode->size() - offset;
    if(lengthLeft > 0) {
        if(lengthLeft > 4096)
            lengthLeft = 4096;
        memcpy((void*)ptr, (u8_t*)vnode_data->ptr + 512 + offset, lengthLeft);
        if(lengthLeft < 4096)
            memset((u8_t*)ptr + lengthLeft, 0, 4096 - lengthLeft);
    } else {
        memset((u8_t*)ptr, 0, 4096);
    }

    pager.unmap(ptr, 1);
    pager.unlock();

    if(mapping.shared()) {
        vnode->f_shared_pages.insert({ offset, page });
    }

    return page;
}

