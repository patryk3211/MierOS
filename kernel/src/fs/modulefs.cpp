#include <fs/modulefs.hpp>
#include <modules/module_manager.hpp>
#include <fs/modulefs_functions.hpp>
#include <errno.h>

using namespace kernel;

ModuleFilesystem::ModuleFilesystem(u16_t major, u16_t minor) : major(major), minor(minor) {
    auto* func = get_module_symbol<fs_function_table>(major, "fs_func_tab");
    if(func == 0) panic("Filesystem module does not have a function table!");
    if(func->set_fs_object == 0) panic("Filesystem module does not implement set_fs_object function! (Required for any filesystem)");

    func->set_fs_object(minor, this);
}

ModuleFilesystem::~ModuleFilesystem() { }

ModuleVNodeDataStorage::ModuleVNodeDataStorage(u16_t major) : major(major) { }

ModuleVNodeDataStorage::~ModuleVNodeDataStorage() {
    auto* func = get_module_symbol<fs_function_table>(major, "fs_func_tab")->fs_data_destroy;
    if(func == 0) panic("Filesystem module does not implement fs_data_destroy function! (Required for any filesystem)");
    func(*this);
}

ValueOrError<void> ModuleFilesystem::umount() {
    auto* func = get_module_symbol<fs_function_table>(major, "fs_func_tab")->umount;
    return func(minor);
}

ValueOrError<std::SharedPtr<VNode>> ModuleFilesystem::get_file(std::SharedPtr<VNode> root, const char* path, FilesystemFlags flags) {
    auto* func = get_module_symbol<fs_function_table>(major, "fs_func_tab")->get_file;
    if(func != 0) return func(minor, root, path, flags);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<std::List<std::SharedPtr<VNode>>> ModuleFilesystem::get_files(std::SharedPtr<VNode> root, const char* path, FilesystemFlags flags) {
    auto* func = get_module_symbol<fs_function_table>(major, "fs_func_tab")->get_files;
    if(func != 0) return func(minor, root, path, flags);
    else return ERR_UNIMPLEMENTED;
}
    
ValueOrError<void> ModuleFilesystem::open(FileStream* stream, int mode) {
    auto* func = get_module_symbol<fs_function_table>(major, "fs_func_tab")->open;
    if(func != 0) return func(minor, stream, mode);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<void> ModuleFilesystem::close(FileStream* stream) {
    auto* func = get_module_symbol<fs_function_table>(major, "fs_func_tab")->close;
    if(func != 0) return func(minor, stream);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<size_t> ModuleFilesystem::read(FileStream* stream, void* buffer, size_t length) {
    auto* func = get_module_symbol<fs_function_table>(major, "fs_func_tab")->read;
    if(func != 0) return func(minor, stream, buffer, length);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<size_t> ModuleFilesystem::write(FileStream* stream, const void* buffer, size_t length) {
    auto* func = get_module_symbol<fs_function_table>(major, "fs_func_tab")->write;
    if(func != 0) return func(minor, stream, buffer, length);
    else return ERR_UNIMPLEMENTED;
}

ValueOrError<size_t> ModuleFilesystem::seek(FileStream* stream, size_t position, int mode) {
    auto* func = get_module_symbol<fs_function_table>(major, "fs_func_tab")->seek;
    if(func != 0) return func(minor, stream, position, mode);
    else return ERR_UNIMPLEMENTED;
}
