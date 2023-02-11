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

    EventManager::get().register_handler(EVENT_LOAD_MODULE, &handle_load_event);
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
    return mod->major();
}

ValueOrError<u16_t> ModuleManager::load_module(const std::String<>& name, const char** args) {
    // Look in module cache
    auto cacheHit = f_module_cache.at(name);
    if(cacheHit)
        return *cacheHit;

    size_t argc = 0;
    size_t argSize = 0;
    while(args[argc]) {
        argSize += strlen(args[argc]);
        ++argc;
    }

    size_t eventSize = sizeof(UEvent) + sizeof(UEventArg) * 2;
    eventSize += argSize + 1;
    eventSize += name.length() + 1;

    u8_t eventStorage[eventSize];

    // Raise a uevent
    UEvent* event = (UEvent*)eventStorage;
    event->type = UEVENT_LOAD_MODULE;
    event->size = eventSize;
    event->argc = 1;

    event->argv[0].size = name.length() + 1;
    event->argv[0].type = UEVENT_ARGT_STR;
    strcpy(event->argv[0].name, "MODULE_NAME");
    memcpy(event->argv[0].value, name.c_str(), name.length() + 1);

    event->argv[1].size = argSize + 1;
    event->argv[1].type = UEVENT_ARGT_STR;
    strcpy(event->argv[1].name, "MODULE_ARGS");

    size_t offset = 0;
    char* storage = (char*)event->argv[1].value;
    for(size_t i = 0; i < argc; ++i) {
        strcpy(storage + offset, args[argc]);
        offset += strlen(args[argc]);
        storage[offset++] = ',';
    }

    storage[offset - 1] = 0;

    s64_t status = (s64_t)EventManager::get().raise_wait_uevent(event);
    if(status < 0)
        return (err_t)(-status);

    return (u16_t)status;
}

Module* ModuleManager::get_module(u16_t major) {
    auto modOpt = f_module_map.at(major);
    return modOpt ? *modOpt : 0;
}

ModuleManager& ModuleManager::get() {
    ASSERT_F(s_instance != 0, "Trying to use the module manager before it was initialized!");
    return *s_instance;
}

typedef void (*load_cb_t)(Module*, void*);
void ModuleManager::handle_load_event(Event& event) {
    auto* load_arg = event.get_arg<const char*>();

    load_cb_t cb = event.get_arg<load_cb_t>();
    void* cb_arg = event.get_arg<void*>();

    auto** argv = event.get_arg<const char**>();

    auto major = s_instance->load_module(std::String<>(load_arg), argv);
    if(!major) {
        kprintf("[%T] (Kernel) Module init failed with code %d\n", major.errno());
        event.set_status(major.errno());
        return;
    }

    auto mod = s_instance->get_module(*major);

    if(cb != 0) {
        // Fire the callback
        cb(mod, cb_arg);
    }

    event.set_status(0);
}

