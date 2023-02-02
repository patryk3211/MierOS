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

    public:
        ModuleManager();
        ~ModuleManager();

        ValueOrError<u16_t> load_module(void* modImage, const char** args);
        ValueOrError<u16_t> load_module(const std::String<>& name, const char** args);
        ValueOrError<u16_t> find_module(const std::String<>& name);

        Module* get_module(u16_t major);

        static ModuleManager& get();

    private:
        u16_t generate_major();

        static void handle_load_event(Event& event);
    };
}
