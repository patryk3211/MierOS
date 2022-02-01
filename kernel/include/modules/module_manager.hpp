#pragma once

#include <types.h>
#include <modules/module.hpp>

namespace kernel {
    u16_t add_preloaded_module(void* file);

    void init_modules(const char* init_signal, void* init_struct);
    int init_module(u16_t major, void* init_struct);

    Module* get_module(u16_t major);
}
