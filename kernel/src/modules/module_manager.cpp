#include <modules/module_manager.hpp>
#include <unordered_map.hpp>
#include <modules/module.hpp>
#include <modules/module_header.h>
#include <defines.h>
#include <dmesg.h>
#include <string.hpp>
#include <locking/spinlock.hpp>
#include <locking/locker.hpp>
#include <list.hpp>

using namespace kernel;

// Map of all currently usable modules
NO_EXPORT std::UnorderedMap<u16_t, Module*> module_map;
// Map of module names to major numbers
NO_EXPORT std::UnorderedMap<std::String<>, u16_t> name_map;

// Map of module paths to major numbers
NO_EXPORT std::UnorderedMap<std::String<>, u16_t> path_map;

struct module_index_list_entry {
    bool is_fallback;
    std::String<> path;
};

struct module_index_entry {
    std::String<> init_signal;
    std::List<module_index_list_entry> modules;
};

// List of init signals with module paths
NO_EXPORT std::List<module_index_entry> module_index;

NO_EXPORT u16_t potential_major = 1;
NO_EXPORT SpinLock major_lock;
NO_EXPORT std::List<std::String<>> inits;
NO_EXPORT bool has_disk_modules;

u16_t find_major() {
    Locker lock(major_lock);
    while(true) {
        if(potential_major == 0) panic("Ran out of major numbers.\n");
        auto mapped_mod = module_map.at(potential_major);
        if(!mapped_mod) return potential_major++;
        ++potential_major;
    }
}

u16_t kernel::add_preloaded_module(void* file) {
    ASSERT_F(file != 0, "Cannot pass a null file to preloaded modules");

    Module* mod = new Module(file, 0);
    auto hdr_sec = mod->get_section(".modulehdr");
    if(!hdr_sec) {
        dmesg("(Kernel) Unable to find .modulehdr section in provided file");
        return 0;
    }
    module_header* header = (module_header*)hdr_sec->address;
    u16_t major = 0;
    if(header->preferred_major != 0) {
        auto mapped_mod = module_map.at(header->preferred_major);
        if(!mapped_mod) major = header->preferred_major;
        else major = find_major();
    } else major = find_major();

    mod->major_num = major;
    module_map.insert({ major, mod });
    name_map.insert({ (const char*)header->name_ptr, major });

    auto module_path = std::String<>("/init/") + (const char*)header->name_ptr;
    path_map.insert({ module_path, major });
    for(char* signal = (char*)header->init_on_ptr; *signal != 0; signal += strlen(signal)+1) {
        for(auto& entry : module_index) {
            if(entry.init_signal == signal) {
                entry.modules.push_back({ (bool)(header->flags & 1), module_path });
                goto found;
            }
        } {
            std::List<module_index_list_entry> mods;
            mods.push_back({ (bool)(header->flags & 1), module_path });
            module_index.push_back({ signal, mods });
        }
    found:;
    }

    return major;
}

u16_t kernel::init_modules(const char* init_signal, void* init_struct) {
    ASSERT_F(init_signal != 0, "Cannot pass a null init signal");

    std::List<module_index_list_entry> possible_modules;
    for(auto& entry : module_index) {
        if(strmatch(entry.init_signal.c_str(), init_signal)) {
            for(auto& list_entry : entry.modules) {
                possible_modules.push_back(list_entry);
            }
        }
    }

    u16_t return_major = 0;

    bool fallback_required = true;
    std::List<std::String<>> potential_fallbacks;
    for(auto entry : possible_modules) {
        if(entry.is_fallback) {
            // This is a fallback module
            if(fallback_required) potential_fallbacks.push_back(entry.path);
        } else {
            if(fallback_required) {
                fallback_required = false;
                potential_fallbacks.clear();
            }
            auto by_path = path_map.at(entry.path);
            if(by_path) {
                // The module is loaded
                auto module = module_map.at(*by_path);
                ASSERT_F(module, "If a module is in path index then it should also be in the map");
                (*module)->init(init_struct);
                if(return_major == 0) return_major = (*module)->major();
            } else {
                // We have to load the module from disk
                dmesg("(Kernel] TODO: [31.01.2022) We have to load the module from disk");
            }
        }
    }
    if(fallback_required) {
        for(auto& path : potential_fallbacks) {
            auto by_path = path_map.at(path);
            if(by_path) {
                // The module is loaded
                auto module = module_map.at(*by_path);
                ASSERT_F(module, "If a module is in path index then it should also be in the map");
                (*module)->init(init_struct);
                if(return_major == 0) return_major = (*module)->major();
            } else {
                // We have to load the module from disk
                dmesg("(Kernel] TODO: [31.01.2022) We have to load the module from disk");
            }
        }
    }

    if(!has_disk_modules) inits.push_back(init_signal);
    return return_major;
}

int kernel::init_module(u16_t major, void* init_struct) {
    auto mapped = module_map.at(major);
    return mapped ? (*mapped)->init(init_struct) : -1;
}

Module* kernel::get_module(u16_t major) {
    auto mapped = module_map.at(major);
    return mapped ? *mapped : 0;
}
