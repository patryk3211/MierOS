#pragma once

#include <vector.hpp>
#include <elf.h>

namespace kernel {
    class Module {
        std::Vector<Elf64_Section> sections;

        virtaddr_t address_base;
    public:
        Module(void* elf_file);
        ~Module();
    };
}
