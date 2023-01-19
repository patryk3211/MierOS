#include <modules/module_manager.hpp>

#include <locking/locker.hpp>
#include <defines.h>
#include <assert.h>
#include <event/event_manager.hpp>
#include <event/kernel_events.hpp>

using namespace kernel;

ModuleManager* ModuleManager::s_instance = 0;

ModuleManager::ModuleManager() {
    f_next_major = 1;

    s_instance = this;

    EventManager::get().register_handler(EVENT_LOAD_MODULE, &handle_load_event);
}

ModuleManager::~ModuleManager() { }

u16_t ModuleManager::generate_major() {
    Locker locker(f_lock);
    return f_next_major++;
}

void ModuleManager::preload_module(void* file) {
    auto major = generate_major();
    auto* mod = new Module(major);

    mod->preload(file);
    f_module_map.insert({ major, mod });
}

u16_t ModuleManager::find_module(const std::String<>& name) {
    // Look in loaded modules
    for(auto [major, mod] : f_module_map) {
        if(mod->name() == name) {
            if(!mod->is_loaded())
                mod->load(0);
            return major;
        }
    }

    // Look in the aliases
    for(auto& alias : f_module_aliases) {
        if(strmatch(alias.f_alias.c_str(), name.c_str())) {
            // Call find_module again, this time with the real module name
            return find_module(alias.f_module_name);
        }
    }

    // Try to load module from disk

    return 0;
}

Module* ModuleManager::get_module(u16_t major) {
    auto modOpt = f_module_map.at(major);
    return modOpt ? *modOpt : 0;
}

ModuleManager& ModuleManager::get() {
    ASSERT_F(s_instance != 0, "Trying to use the module manager before it was initialized!");
    return *s_instance;
}

void ModuleManager::handle_load_event(Event& event) {
    char* load_arg = event.get_arg<char*>();

    u16_t major_num = s_instance->find_module(load_arg);
    if(!major_num) return;

    u8_t argc = event.get_arg<u8_t>();
    char* argv[argc + 1];
    argv[argc] = 0;

    for(int i = 0; i < argc; ++i)
        argv[i] = event.get_arg<char*>();

    Module* mod = s_instance->get_module(major_num);
    int status = mod->init(argc, argv);

    if(status) {
        kprintf("[%T] (Kernel) Module init failed with code %d\n", status);
    }
}
