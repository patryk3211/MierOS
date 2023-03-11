#include <streams/directorystream.hpp>
#include <fs/vfs.hpp>

using namespace kernel;

DirectoryStream::DirectoryStream(const VNodePtr& directory)
    : FileStream(directory)
    , f_current_entry(f_directories.end()) {
}

ValueOrError<void> DirectoryStream::open() {
    auto result = VFS::instance()->get_files(f_file, "", { });
    if(!result)
        return result.errno();

    f_directories = *result;
    f_current_entry = f_directories.begin();
    f_open = true;
    return { };
}

#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12

struct dirent {
    long inode;
    long offset;
    unsigned short reclen;
    unsigned char type;

    char name[];
};

ValueOrError<size_t> DirectoryStream::read(void* buffer, size_t length) {
    if(!f_open)
        return EBADF;

    size_t readBytes = 0;
    
    u8_t* byteBuffer = (u8_t*)buffer;
    while(readBytes < length && f_current_entry != f_directories.end()) {
        dirent* currentEntry = (dirent*)(byteBuffer + readBytes);
        auto& node = *f_current_entry++;
        
        size_t recSize = node->name().length() + sizeof(dirent) + 1;
        if(recSize > (length - readBytes))
            break;

        currentEntry->reclen = recSize;
        currentEntry->inode = 0;
        currentEntry->offset = 0;
        
        switch(node->type()) {
            case VNode::FILE:
                currentEntry->type = DT_REG;
                break;
            case VNode::MOUNT:
            case VNode::DIRECTORY:
                currentEntry->type = DT_DIR;
                break;
            case VNode::BLOCK_DEVICE:
                currentEntry->type = DT_BLK;
                break;
            case VNode::CHARACTER_DEVICE:
                currentEntry->type = DT_CHR;
                break;
            case VNode::LINK:
                currentEntry->type = DT_LNK;
                break;
            default:
                currentEntry->type = DT_UNKNOWN;
                break;
        }

        memcpy(currentEntry->name, node->name().c_str(), node->name().length() + 1);
        readBytes += recSize;
    }

    return readBytes;
}

ValueOrError<size_t> DirectoryStream::write(const void*, size_t) {
    return ENOTSUP;
}

ValueOrError<size_t> DirectoryStream::seek(size_t, int) {
    // For now the dirstream is a pipelike interface,
    // I will implement the seeking later
    return ESPIPE;
}

