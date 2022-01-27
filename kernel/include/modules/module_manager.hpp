#pragma once

#include <types.h>

namespace kernel {
    void init_module_manager();
    u16_t add_preloaded_module(const char* name, void* file);

    int init_module(u16_t major, u32_t* additional_args);
    virtaddr_t get_module_symbol_addr(u16_t major, const char* symbol_name);

    template<typename Ret, typename... Args> Ret run_function(u16_t major, const char* func_name, Args... args) {
        typedef Ret func_t(Args...);
        return ((func_t*)get_module_symbol_addr(major, func_name))(args...);
    }
}
