#pragma once

#include <elf.h>
#include <locking/spinlock.hpp>
#include <optional.hpp>
#include <range_map.hpp>
#include <string.hpp>
#include <vector.hpp>

namespace kernel {
    class Module {
    public:
        struct Section {
            std::String<> name;
            u64_t address;
            u64_t size;
            u32_t type;
            u32_t flags;
            u32_t entry_size;
        };

    private:
        struct ThreadModuleSetter {
            Module* old_mod;
            ThreadModuleSetter(Module* mod);
            ~ThreadModuleSetter();
        };

        u16_t major_num;

        std::Vector<Section> sections;
        std::RangeMap<virtaddr_t> allocated_ranges;

        virtaddr_t address_base;

        Section* symbol_table;
        Section* symbol_names;

        char* init_signals;
        u16_t f_flags;
        bool initialized;

        SpinLock locker;

    public:
        Module(void* elf_file, u16_t major_num);
        ~Module();

        int init(void* init_struct);
        bool is_appropriate(const char* init_signal);

        void lock() { locker.lock(); }
        void unlock() { locker.unlock(); }

        u16_t flags() { return f_flags; }

        virtaddr_t base() { return address_base; }

        template<typename Ret, typename... Args> Ret run_function(const char* func_name, Args... args) {
            ThreadModuleSetter setter(this);
            return ((Ret(*)(Args...))(address_base + get_symbol(func_name)->addr))(args...);
        }

        virtaddr_t get_symbol_addr(const char* name) {
            return address_base + get_symbol(name)->addr;
        }

        u16_t major() { return major_num; }

    private:
        void link();
        void run_ctors();
        void run_dtors();

        std::OptionalRef<Section> get_section(const char* name);
        std::OptionalRef<Elf64_Symbol> get_symbol(const char* name);
        std::OptionalRef<Elf64_Symbol> get_symbol(size_t index);

        friend u16_t add_preloaded_module(void* file);
    };
}
