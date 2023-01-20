#pragma once

#include <modules/module.hpp>
#include <locking/spinlock.hpp>
#include <types.h>
#include <event/event.hpp>

namespace kernel {
    class ModuleManager {
        static ModuleManager* s_instance;

        u16_t f_next_major;
        SpinLock f_lock;

        std::UnorderedMap<u16_t, Module*> f_module_map;

        struct ModuleAlias {
            std::String<> f_alias;
            std::String<> f_module_name;
        };

        std::List<ModuleAlias> f_module_aliases;

    public:
        ModuleManager();
        ~ModuleManager();

        void preload_module(void* file);
        u16_t find_module(const std::String<>& name);

        Module* get_module(u16_t major);

        void reload_modules();

        static ModuleManager& get();

    private:
        u16_t generate_major();

        static void handle_load_event(Event& event);
    };
}
