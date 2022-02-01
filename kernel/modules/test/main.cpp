#include <dmesg.h>
#include <vector.hpp>
#include <modules/module_header.h>
#include <defines.h>

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

MODULE_HEADER char header_mod_name[] = "test";
MODULE_HEADER char init_on[] = "TEST\0";

std::Vector<int> vec = std::Vector<int>(10, 1);

extern "C" int init() {
    dmesg("[Test Mod] Testing\n");
    vec.push_back(105);
    return vec[0] + vec[10];
}

extern "C" int destroy() {
    return 0;
}
