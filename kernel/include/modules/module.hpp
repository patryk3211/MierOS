#pragma once

#include <types.h>
#include <unordered_map.hpp>
#include <string.hpp>

namespace kernel {
    class Module {
        u16_t f_major_num;

        virtaddr_t f_base_addr;
        std::UnorderedMap<std::String<>, u64_t> f_symbol_map;

    public:
        Module(u16_t major);
        ~Module();

        int load(void* file);

        u16_t major();

        template<typename Ret, typename... Args> Ret run_function(const char* name, Args... args) {
            void* sym = get_symbol_ptr(name);
            return ((Ret(*)(Args...))sym)(args...);
        }

        void* get_symbol_ptr(const char* name);
    };
}
