#pragma once

#include <modules/module.hpp>
#include <types.h>

namespace kernel {
    u16_t add_preloaded_module(void* file);

    u16_t init_modules(const char* init_signal, void* init_struct);
    int init_module(u16_t major, void* init_struct);

    Module* get_module(u16_t major);

    template<typename Ret, typename... Args> Ret run_module_func(u16_t major, const char* func_name, Args... args) {
        return get_module(major)->run_function(func_name, args...);
    }

    template<typename T> T* get_module_symbol(u16_t major, const char* name) {
        return (T*)get_module(major)->get_symbol_addr(name);
    }
}
