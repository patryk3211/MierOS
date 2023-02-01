#include <modules/module_manager.hpp>

#include <locking/locker.hpp>
#include <defines.h>
#include <assert.h>
#include <event/event_manager.hpp>
#include <event/kernel_events.hpp>
#include <modules/module_header.h>
#include <string.h>

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

ValueOrError<u16_t> ModuleManager::find_module(const std::String<>& name, const char** args) {
    // Look in module cache
    auto cacheHit = f_module_cache.at(name);
    if(cacheHit)
        return *cacheHit;

    // Look in loaded modules
    /*for(auto [major, mod] : f_module_map) {
        if(mod->name() == name) {
            return major;
        }
    }*/

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
    event->f_event_type = UEVENT_LOAD_MODULE;
    event->f_event_size = eventSize;
    event->f_arg_count = 1;

    event->f_args[0].f_size = name.length() + 1;
    event->f_args[0].f_type = UEVENT_ARGT_STR;
    strcpy(event->f_args[0].f_name, "MODULE_NAME");
    memcpy(event->f_args[0].f_value, name.c_str(), name.length() + 1);

    event->f_args[1].f_size = argSize + 1;
    event->f_args[1].f_type = UEVENT_ARGT_STR;
    strcpy(event->f_args[1].f_name, "MODULE_ARGS");

    size_t offset = 0;
    char* storage = (char*)event->f_args[1].f_value;
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

    // Look in the aliases
    /*for(auto& alias : f_module_aliases) {
        if(strmatch(alias.f_alias.c_str(), name.c_str())) {
            // Call find_module again, this time with the real module name
            return find_module(alias.f_module_name);
        }
    }

    // Try to load module from disk
    auto idxEntry = f_module_index.at(name);
    if(idxEntry) {
        auto node = *idxEntry;
        u8_t* buffer = new u8_t[node->f_size];

        FileStream stream(node);
        stream.open(FILE_OPEN_MODE_READ);
        stream.read(buffer, node->f_size);

        Module* mod = new Module(generate_major());
        auto status = mod->load(buffer);
        delete[] buffer;

        if(status) {
            delete mod;
            return status.errno();
        }

        f_module_map.insert({ mod->major(), mod });
        return mod->major();
    }*/
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

    auto major = s_instance->find_module(load_arg, argv);
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

// void ModuleManager::reload_modules() {
//     // Create an index of modules
//     f_module_index.clear();
//     f_module_aliases.clear();
//
//     auto moduleDir = VFS::instance()->get_file(nullptr, "/lib/modules", { .resolve_link = true, .follow_links = true });
//
//     if(!moduleDir)
//         panic("Could not find the modules directory");
//     if((*moduleDir)->type() != VNode::DIRECTORY)
//         panic("/lib/modules is not a directory");
//
//     process_directory(*moduleDir);
//
//     auto aliases = VFS::instance()->get_file(nullptr, "/etc/modules.alias", { .resolve_link = true, .follow_links = true });
//     if(aliases && (*aliases)->type() == VNode::FILE) {
//         // The alias file is present
//         FileStream stream(*aliases);
//         stream.open(FILE_OPEN_MODE_READ);
//
//         enum State {
//             READ_NEXT,
//             READ_ALIAS_NAME,
//             READ_MODULE_NAME,
//             PRE_ALIAS_NAME,
//             PRE_MODULE_NAME,
//             SKIP_COMMENT
//         };
//
//         State nextState = READ_NEXT;
//         State returnState = PRE_ALIAS_NAME;
//
//         char c;
//         std::String<> aliasName;
//         std::String<> modName;
//
//         bool continueParsing = true;
//         while(continueParsing) {
//             switch(nextState) {
//                 case READ_NEXT:
//                     if(stream.read(&c, sizeof(c)) != sizeof(c))
//                         c = 0;
//                     nextState = returnState;
//                     break;
//                 case PRE_ALIAS_NAME:
//                     if(c == '#') {
//                         nextState = SKIP_COMMENT;
//                     } else if(c == '\n' || c == 0) {
//                         continueParsing = false;
//                     } else if(c != ' ') {
//                         nextState = READ_ALIAS_NAME;
//                     } else {
//                         nextState = READ_NEXT;
//                         returnState = PRE_ALIAS_NAME;
//                     }
//                     break;
//                 case READ_ALIAS_NAME:
//                     if(c == '#') {
//                         nextState = SKIP_COMMENT;
//                     } else if(c == ' ') {
//                         nextState = READ_NEXT;
//                         returnState = PRE_MODULE_NAME;
//                     } else if(c == '\n' || c == 0) {
//                         continueParsing = false;
//                     } else {
//                         aliasName += c;
//                         nextState = READ_NEXT;
//                         returnState = READ_ALIAS_NAME;
//                     }
//                     break;
//                 case PRE_MODULE_NAME:
//                     if(c == '#') {
//                         nextState = SKIP_COMMENT;
//                     } else if(c == '\n' || c == 0) {
//                         continueParsing = false;
//                     } else if(c != ' ') {
//                         nextState = READ_MODULE_NAME;
//                     } else {
//                         nextState = READ_NEXT;
//                         returnState = PRE_MODULE_NAME;
//                     }
//                     break;
//                 case READ_MODULE_NAME:
//                     if(c == '\n' || c == 0 || c == '#' || c == ' ') {
//                         f_module_aliases.push_back({ std::move(aliasName), std::move(modName) });
//                         aliasName = "";
//                         modName = "";
//
//                         nextState = c == '#' ? SKIP_COMMENT : READ_NEXT;
//                         returnState = PRE_ALIAS_NAME;
//                         continueParsing = c != 0;
//                     } else {
//                         modName += c;
//                         nextState = READ_NEXT;
//                         returnState = READ_MODULE_NAME;
//                     }
//                     break;
//                 case SKIP_COMMENT:
//                     if(c == '\n') {
//                         nextState = READ_NEXT;
//                         returnState = PRE_ALIAS_NAME;
//                     } else if(c == 0) {
//                         continueParsing = false;
//                     } else {
//                         nextState = READ_NEXT;
//                         returnState = SKIP_COMMENT;
//                     }
//                     break;
//             }
//         }
//     }
// }
//
// void ModuleManager::run_init_modules() {
//     auto file = VFS::instance()->get_file(nullptr, "/etc/modules.init", { .resolve_link = true, .follow_links = true });
//     if(file) {
//         FileStream stream(*file);
//         stream.open(FILE_OPEN_MODE_READ);
//
//         std::String<> name;
//         bool parse = true;
//         while(parse) {
//             char c;
//             if(stream.read(&c, sizeof(c)) != sizeof(c)) {
//                 parse = false;
//                 c = '\n';
//             }
//
//             if(c == '\n') {
//                 // Run the module
//                 Event* e = new Event(EVENT_LOAD_MODULE, name.c_str(), 0, 0, 0);
//                 EventManager::get().raise(e);
//                 name = "";
//             } else {
//                 name += c;
//             }
//         }
//     }
// }
//
// // Skip file if an EOF occurs
// #define CHECKED_READ(dest, size) \
//     if(stream.read((dest), (size)) != (size)) \
//         return;
//
// static const char ELF_IDENT[] = { 0x7F, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
// static const char MOD_IDENT[] = MODULE_HEADER_MAGIC;
// void ModuleManager::process_directory(VNodePtr dir) {
//     auto result = VFS::instance()->get_files(dir, "", { .resolve_link = true, .follow_links = true });
//     if(!result) return;
//
//     auto files = *result;
//     for(auto& node : files) {
//         if(node->type() == VNode::DIRECTORY) {
//             // Recursion
//             process_directory(node);
//         } else {
//             // This is a regular file (or atleast it should be)
//             process_file(node);
//         }
//     }
// }
//
// void ModuleManager::process_file(VNodePtr file) {
//     FileStream stream(file);
//     stream.open(FILE_OPEN_MODE_READ);
//
//     Elf64_Header header;
//     CHECKED_READ(&header, sizeof(header));
//
//     if(!memcmp(&header, ELF_IDENT, sizeof(ELF_IDENT))) {
//         // Invalid ELF header, skip file
//         return;
//     }
//
//     stream.seek(header.phdr_offset, SEEK_MODE_BEG);
//     for(size_t i = 0; i < header.phdr_entry_count; ++i) {
//         Elf64_Phdr phdr;
//         CHECKED_READ(&phdr, sizeof(phdr));
//
//         if(phdr.type == PT_NOTE) {
//             // Found the module header location
//             u8_t buffer[phdr.file_size];
//             stream.seek(phdr.offset, SEEK_MODE_BEG);
//             CHECKED_READ(buffer, phdr.file_size);
//
//             module_header* modHead = (module_header*)memfind(buffer, MOD_IDENT, sizeof(MOD_IDENT), phdr.file_size);
//             if(modHead == 0) continue;
//
//             f_module_index.insert({ modHead->mod_name, file });
//             return;
//         }
//     }
// }
