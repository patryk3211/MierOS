#include <errno.h>
#include <fs/modulefs.hpp>
#include <modules/module_manager.hpp>

using namespace kernel;

ModuleFilesystem::ModuleFilesystem(u16_t major, u16_t minor)
    : f_major(major)
    , f_minor(minor) {
    f_driver = get_driver();
    if(f_driver == 0) panic("Filesystem module does not have a function table!");
    if(f_driver->set_fs_object == 0) panic("Filesystem module does not implement set_fs_object function! (Required for any filesystem)");

    f_driver->set_fs_object(minor, this);
}

ModuleFilesystem::~ModuleFilesystem() {
}

FilesystemDriver* ModuleFilesystem::get_driver() {
    auto* mod = ModuleManager::get().get_module(f_major);
    FilesystemDriver* driver = (FilesystemDriver*)mod->get_symbol_ptr("filesystem_driver");
    return driver;
}

#define RUN_FUNC(func, ...) { \
    auto* f = f_driver->func; \
    if(!f) \
        return ERR_UNIMPLEMENTED; \
    return f(f_minor, __VA_ARGS__); \
}

ValueOrError<void> ModuleFilesystem::umount() {
    auto* func = get_driver()->umount;
    return func(f_minor);
}

ValueOrError<VNodePtr> ModuleFilesystem::get_file(VNodePtr root, const char* path, FilesystemFlags flags) {
    RUN_FUNC(get_file, root, path, flags);
}

ValueOrError<std::List<VNodePtr>> ModuleFilesystem::get_files(VNodePtr root, FilesystemFlags flags) {
    RUN_FUNC(get_files, root, flags);
}

ValueOrError<VNodePtr> ModuleFilesystem::resolve_link(VNodePtr link) {
    RUN_FUNC(resolve_link, link);
}

ValueOrError<void> ModuleFilesystem::open(FileStream* stream, int mode) {
    RUN_FUNC(open, stream, mode);
}

ValueOrError<void> ModuleFilesystem::close(FileStream* stream) {
    RUN_FUNC(close, stream);
}

ValueOrError<size_t> ModuleFilesystem::read(FileStream* stream, void* buffer, size_t length) {
    RUN_FUNC(read, stream, buffer, length);
}

ValueOrError<size_t> ModuleFilesystem::write(FileStream* stream, const void* buffer, size_t length) {
    RUN_FUNC(write, stream, buffer, length);
}

ValueOrError<size_t> ModuleFilesystem::seek(FileStream* stream, size_t position, int mode) {
    RUN_FUNC(seek, stream, position, mode);
}

PhysicalPage ModuleFilesystem::resolve_mapping(const FilePage& mapping, virtaddr_t addr) {
    auto* func = f_driver->resolve_mapping;
    if(func)
        return func(f_minor, mapping, addr);
    else
        return nullptr;
}

void ModuleFilesystem::sync_mapping(const MemoryFilePage& mapping) {
    auto* func = f_driver->sync_mapping;
    if(func != 0) func(f_minor, mapping);
}
