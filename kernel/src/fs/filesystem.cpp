#include <fs/filesystem.hpp>
#include <fs/vnode.hpp>

using namespace kernel;

ValueOrError<void> Filesystem::umount() {
    return ERR_UNIMPLEMENTED;
}

ValueOrError<std::SharedPtr<VNode>> Filesystem::get_file(std::SharedPtr<VNode>, const char*, FilesystemFlags) {
    return ERR_UNIMPLEMENTED;
}

ValueOrError<std::List<std::SharedPtr<VNode>>> Filesystem::get_files(std::SharedPtr<VNode>, const char*, FilesystemFlags) {
    return ERR_UNIMPLEMENTED;
}

ValueOrError<VNodePtr> Filesystem::resolve_link(VNodePtr link) {
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

ValueOrError<size_t> Filesystem::write(FileStream*, const void*, size_t) {
    return ERR_UNIMPLEMENTED;
}

ValueOrError<size_t> Filesystem::seek(FileStream*, size_t, int) {
    return ERR_UNIMPLEMENTED;
}

PhysicalPage Filesystem::resolve_mapping(const FilePage&, virtaddr_t) {
    return nullptr;
}

void Filesystem::sync_mapping(const MemoryFilePage&) {

}
