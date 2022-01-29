#include <modules/module_manager.hpp>
#include <unordered_map.hpp>
#include <modules/module.hpp>
#include <defines.h>

using namespace kernel;

NO_EXPORT std::UnorderedMap<u16_t, Module*> module_map;

void kernel::init_module_manager() {
    
}

u16_t kernel::add_preloaded_module(const char* name, void* file) {

}

int kernel::init_module(u16_t major, u32_t* additional_args) {

}

virtaddr_t kernel::get_module_symbol_addr(u16_t major, const char* symbol_name) {

}