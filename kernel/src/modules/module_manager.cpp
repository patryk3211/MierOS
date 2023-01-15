#include <modules/module_manager.hpp>

#include <locking/locker.hpp>
#include <defines.h>
#include <assert.h>

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

void ModuleManager::preload_module(void* file) {
    auto major = generate_major();
    auto* mod = new Module(major);

    int stat = mod->load(file);
    if(stat) {
        kprintf("[%T] (Kernel) Failed to preload a module, code %d\n", stat);
        return;
    }
}

ModuleManager& ModuleManager::get() {
    ASSERT_F(s_instance != 0, "Trying to use the module manager before it was initialized!");
    return *s_instance;
}