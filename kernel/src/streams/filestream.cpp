#include <fs/vnode.hpp>
#include <streams/filestream.hpp>

using namespace kernel;

FileStream::FileStream(const std::SharedPtr<VNode>& file)
    : Stream(STREAM_TYPE_FILE)
    , f_file(file) {
    fs_data = 0;
    f_open = false;
}

FileStream::~FileStream() {
    if(f_open) f_file->filesystem()->close(this);
}

ValueOrError<void> FileStream::open(int mode) {
    auto val = f_file->filesystem()->open(this, mode);
    if(val) f_open = true;
    return val;
}

size_t FileStream::read(void* buffer, size_t length) {
    auto val = f_file->filesystem()->read(this, buffer, length);
    if(val)
        return *val;
    else
        return -1;
}

size_t FileStream::seek(size_t position, int mode) {
    auto val = f_file->filesystem()->seek(this, position, mode);
    if(val)
        return *val;
    else
        return -1;
}

ValueOrError<int> FileStream::ioctl(unsigned long request, void* arg) {
    return f_file->filesystem()->ioctl(this, request, arg);
}

