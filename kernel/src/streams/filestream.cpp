#include <streams/filestream.hpp>
#include <fs/vnode.hpp>

using namespace kernel;

FileStream::FileStream(const std::SharedPtr<VNode>& file) : Stream(STREAM_TYPE_FILE), f_file(file) {
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
