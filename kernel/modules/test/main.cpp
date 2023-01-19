#include <defines.h>
#include <dmesg.h>
#include <modules/module_header.h>
#include <vector.hpp>

MODULE_HEADER static module_header header {
    .magic = MODULE_HEADER_MAGIC,
    .mod_name = "test",
    .dependencies = 0
};

std::Vector<int> vec = std::Vector<int>(10, 1);

extern "C" int init() {
    dmesg("(Test Mod) Testing");
    vec.push_back(105);
    return vec[0] + vec[10];
}

extern "C" int destroy() {
    return 0;
}
