#pragma once

#include <vector.hpp>

namespace kernel {
    class Module {
        std::Vector<Elf64_Section> sections;
    public:
        Module(void* elf_file);
        ~Module();
    };
}
