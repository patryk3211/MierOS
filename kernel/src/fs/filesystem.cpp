#include <fs/filesystem.hpp>
#include <fs/vnode.hpp>

using namespace kernel;

ValueOrError<void> Filesystem::umount() {
    return ENOTSUP;
}

ValueOrError<VNodePtr> Filesystem::get_file(VNodePtr, const char*, FilesystemFlags) {
    return ENOTSUP;
}

ValueOrError<std::List<VNodePtr>> Filesystem::get_files(VNodePtr, FilesystemFlags) {
    return ENOTSUP;
}

ValueOrError<VNodePtr> Filesystem::resolve_link(VNodePtr, int) {
    return ENOTSUP;
}

ValueOrError<void> Filesystem::open(FileStream*, int) {
    return ENOTSUP;
}

ValueOrError<void> Filesystem::close(FileStream*) {
    return ENOTSUP;
}

ValueOrError<size_t> Filesystem::read(FileStream*, void*, size_t) {
    return ENOTSUP;
}

ValueOrError<size_t> Filesystem::write(FileStream*, const void*, size_t) {
    return ENOTSUP;
}

ValueOrError<size_t> Filesystem::seek(FileStream*, size_t, int) {
    return ENOTSUP;
}

PhysicalPage Filesystem::resolve_mapping(const FilePage&, virtaddr_t) {
    return nullptr;
}

void Filesystem::sync_mapping(const MemoryFilePage&) {

}

ValueOrError<int> Filesystem::ioctl(FileStream*, u64_t, void*) {
    return ENOTSUP;
}

// ValueOrError<VNodePtr> Filesystem::link(VNodePtr, const char*, VNodePtr, bool) {
//     return ENOTSUP;
// }

ValueOrError<VNodePtr> Filesystem::mkdir(VNodePtr, const char*) {
    return ENOTSUP;
}

ValueOrError<VNodePtr> Filesystem::symlink(VNodePtr, const char*, const char*) {
    return ENOTSUP;
}

