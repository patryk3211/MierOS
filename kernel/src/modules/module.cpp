#include <modules/module.hpp>

using namespace kernel;

Module::Module(u16_t major)
    : f_major_num(major) {
    f_base_addr = 0;
}

Module::~Module() {

}

int Module::load(void* file) {
    // Do the parsing
}

void* Module::get_symbol_ptr(const char* name) {
    auto res = f_symbol_map.at(name);
    return res ? (void*)(*res + f_base_addr) : 0;
}

u16_t Module::major() {
    return f_major_num;
}
