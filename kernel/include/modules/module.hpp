#pragma once

#include <vector.hpp>
#include <elf.h>
#include <string.hpp>
#include <range_map.hpp>
#include <optional.hpp>

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
    public:
        Module(void* elf_file, u16_t major_num);
        ~Module();

        template<typename Ret, typename... Args> Ret run_function(const char* func_name, Args... args) {
            ThreadModuleSetter setter(this);
            return ((Ret (*)(Args...))(address_base + get_symbol(func_name)->addr))(args...);
        }
    private:
        void link();
        void run_ctors();
        void run_dtors();

        std::OptionalRef<Section> get_section(std::String<> name);
        std::OptionalRef<Elf64_Symbol> get_symbol(std::String<> name);
        std::OptionalRef<Elf64_Symbol> get_symbol(size_t index);
    };
}
