#include <fs/filesystem.hpp>

using namespace kernel;

ValueOrError<void> Filesystem::umount() {
    return ERR_UNIMPLEMENTED;
}

ValueOrError<VNode*> Filesystem::get_file(VNode*, const char*) {
    return ERR_UNIMPLEMENTED;
}

ValueOrError<std::List<VNode*>> Filesystem::get_files(VNode*, const char*) {
    return ERR_UNIMPLEMENTED;
}
    
ValueOrError<void> Filesystem::open(FileStream*, int) {
    return ERR_UNIMPLEMENTED;
}

ValueOrError<void> Filesystem::close(FileStream*) {
    return ERR_UNIMPLEMENTED;
}

ValueOrError<size_t> Filesystem::read(FileStream*, void*, size_t) {
    return ERR_UNIMPLEMENTED;
}

ValueOrError<size_t> Filesystem::write(FileStream*, void*, size_t) {
    return ERR_UNIMPLEMENTED;
}

ValueOrError<size_t> Filesystem::seek(FileStream*, size_t, int) {
    return ERR_UNIMPLEMENTED;
}
