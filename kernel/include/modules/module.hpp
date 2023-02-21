#pragma once

#include <types.h>
#include <unordered_map.hpp>
#include <string.hpp>
#include <list.hpp>
#include <elf.h>
#include <errno.h>

namespace kernel {
    class Module {
        struct Section {
            std::String<> f_name;
            void* f_ptr;
            size_t f_size;
            u64_t f_flags;
            u32_t f_type;
            u64_t f_entry_size;
        };

        u16_t f_major_num;

        virtaddr_t f_base_addr;
        size_t f_page_count;

        std::UnorderedMap<std::String<>, Section> f_section_map;

        size_t f_symbol_count;
        Elf64_Symbol* f_symbols;
        char* f_symbol_names;

        std::String<> f_name;

        // List of major numbers of dependency modules
        std::List<u16_t> f_dependencies;

        bool f_loaded;
        bool f_initialized;

    public:
        Module(u16_t major);
        ~Module();

        ValueOrError<void> load(void* file);
        bool is_loaded();

        u16_t major();

        int preinit();
        template<typename... Args> int init(Args... args) {
            if(f_initialized)
                return 0;
            f_initialized = true;
            int status = preinit();
            if(status)
                return status;
            return run_function<int>("init", args...);
        }
        bool is_initialized();

        template<typename Ret, typename... Args> Ret run_function(const char* name, Args... args) {
            void* sym = get_symbol_ptr(name);
            return ((Ret(*)(Args...))sym)(args...);
        }

        Elf64_Symbol* get_symbol(size_t index);
        Elf64_Symbol* get_symbol(const char* name);

        void* get_symbol_ptr(const char* name);

        const std::String<>& name() const;

    private:
        int link();

        int load_symbols();

        int parse_module_header();
        int parse_module_header_v1(void* headerPtr, size_t size);

        void print_error(int errCode);

        void constructors();
        void destructors();
    };
}
