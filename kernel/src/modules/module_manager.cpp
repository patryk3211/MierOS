#include <modules/module_manager.hpp>

#include <locking/locker.hpp>
#include <defines.h>
#include <assert.h>
#include <event/event_manager.hpp>
#include <event/kernel_events.hpp>
#include <modules/module_header.h>
#include <stdlib.h>

using namespace kernel;

ModuleManager* ModuleManager::s_instance = 0;

ModuleManager::ModuleManager() {
    f_next_major = 1;

    s_instance = this;
}

ModuleManager::~ModuleManager() { }

u16_t ModuleManager::generate_major() {
    Locker locker(f_lock);
    return f_next_major++;
}

ValueOrError<u16_t> ModuleManager::find_module(const std::String<>& name) {
    auto cacheHit = f_module_cache.at(name);
    if(cacheHit)
        return *cacheHit;
    return ENOEXEC;
}

ValueOrError<u16_t> ModuleManager::load_module(void* modImage, const char** args) {
    auto* mod = new Module(generate_major());

    // Load module
    auto status = mod->load(modImage);
    if(!status)
        return status.errno();

    // Count args
    size_t argc = 0;
    if(args) {
        while(args[argc])
            ++argc;
    }

    int result = mod->init(argc, args);
    if(result) return (err_t)result;

    f_module_cache.insert({ mod->name(), mod->major() });
    f_module_map.insert({ mod->major(), mod });
    return mod->major();
}

Module* ModuleManager::get_module(u16_t major) {
    auto modOpt = f_module_map.at(major);
    return modOpt ? *modOpt : 0;
}

ModuleManager& ModuleManager::get() {
    ASSERT_F(s_instance != 0, "Trying to use the module manager before it was initialized!");
    return *s_instance;
}

