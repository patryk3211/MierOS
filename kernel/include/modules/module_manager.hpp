#pragma once

#include <modules/module.hpp>
#include <locking/spinlock.hpp>
#include <types.h>
#include <event/event.hpp>
#include <fs/vfs.hpp>

namespace kernel {
    class ModuleManager {
        static ModuleManager* s_instance;

        u16_t f_next_major;
        SpinLock f_lock;

        std::UnorderedMap<u16_t, Module*> f_module_map;
        std::UnorderedMap<std::String<>, u16_t> f_module_cache;

        /*struct ModuleAlias {
            std::String<> f_alias;
            std::String<> f_module_name;
        };
        std::List<ModuleAlias> f_module_aliases;

        std::UnorderedMap<std::String<>, VNodePtr> f_module_index;*/

    public:
        ModuleManager();
        ~ModuleManager();

        ValueOrError<u16_t> find_module(const std::String<>& name, const char** args);

        Module* get_module(u16_t major);

        //void reload_modules();
        //void run_init_modules();

        static ModuleManager& get();

    private:
        u16_t generate_major();

        //void process_directory(VNodePtr dir);
        //void process_file(VNodePtr file);

        static void handle_load_event(Event& event);
    };
}
