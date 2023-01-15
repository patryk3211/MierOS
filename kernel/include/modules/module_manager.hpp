#pragma once

#include <modules/module.hpp>
#include <locking/spinlock.hpp>
#include <types.h>

namespace kernel {
    class ModuleManager {
        static ModuleManager* s_instance;

        u16_t f_next_major;
        SpinLock f_lock;

        std::UnorderedMap<u16_t, Module*> f_module_map;

    public:
        ModuleManager();
        ~ModuleManager();

        void preload_module(void* file);

        static ModuleManager& get();

    private:
        u16_t generate_major();
    };
}
