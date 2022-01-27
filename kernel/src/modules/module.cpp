#include <modules/module.hpp>
#include <elf.h>
#include <stdlib.h>
#include <dmesg.h>

using namespace kernel;

const u8_t ELF_HEADER[] = { 0x7F, 'E', 'L', 'F', 2, 1, 1, 0 };
Module::Module(void* elf_file) {
    u8_t* elf_file_c = (u8_t*)elf_file;
    Elf64_Header* elf_header = (Elf64_Header*)elf_file;
    if(!memcmp(elf_header, ELF_HEADER, 8)) {
        dmesg("[Kernel] Invalid module ELF header!\n");
        return;
    }

    
}

Module::~Module() {

}
