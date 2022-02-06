#include <modules/module_header.h>
#include <defines.h>
#include <fs/modulefs_functions.hpp>

using namespace kernel;

extern char header_mod_name[];
extern char init_on[];
MODULE_HEADER static module_header header {
    .header_version = 1, // Header Version 1
    .reserved1 = 0,
    .preferred_major = 0, // No preferred major number value
    .flags = 0,
    .reserved2 = 0,
    .dependencies_ptr = 0, // No dependencies
    .name_ptr = (u64_t)&header_mod_name, // Name
    .init_on_ptr = (u64_t)&init_on // Initialize on
};

MODULE_HEADER char header_mod_name[] = "ext2";
MODULE_HEADER char init_on[] = "FS-ext2";

ValueOrError<u16_t> mount(VNode* fs_file);

static fs_function_table fs_func_tab {
    .mount = &mount
};

extern "C" int init() {

}

extern "C" int destroy() {

}

ValueOrError<u16_t> mount(VNode* fs_file) {
    
}
