#include <modules/module.hpp>
#include <elf.h>
#include <dmesg.h>

using namespace kernel;

Module::Module(u16_t major)
    : f_major_num(major) {
    f_base_addr = 0;
}

Module::~Module() {

}

const char ELF_IDENT[] = { 0x7F, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const char MOD_IDENT[] = { 'M', 'O', 'D', 'H', 'D', 'R', 'V', '1' };
int Module::load(void* file) {
    // Do the parsing
    Elf64_Header* elfHeader = (Elf64_Header*)file;
    if(!memcmp(elfHeader, ELF_IDENT, sizeof(ELF_IDENT))) {
        dmesg("(Kernel) Invalid module format");
        return 1;
    }

    Elf64_Section* sections = (Elf64_Section*)((u8_t*)file + elfHeader->sect_offset);
    u8_t* sectionNames = (u8_t*)file + sections[elfHeader->sect_name_idx].offset;

    int status = parse_module_header(file);
    if(status) return status;

    // We have parsed the module header and can now load the module into memory

    // First let's save the sections for later use
    for(int i = 0; i < elfHeader->sect_entry_count; ++i) {
        // Only save the section if it is allocated
        if(sections[i].flags & SHF_ALLOC) {

        }
    }

    Elf64_Phdr* programHeaders = (Elf64_Phdr*)((u8_t*)file + elfHeader->phdr_offset);
}

int Module::parse_module_header(void* file) {
    Elf64_Header* elfHeader = (Elf64_Header*)file;
    Elf64_Section* sections = (Elf64_Section*)((u8_t*)file + elfHeader->sect_offset);
    u8_t* sectionNames = (u8_t*)file + sections[elfHeader->sect_name_idx].offset;

    for(int i = 0; i < elfHeader->sect_entry_count; ++i) {
        if(strcmp((char*)(sectionNames + sections[i].name_idx), ".modulehdr")) {
            if(memcmp((u8_t*)file + sections[i].offset, MOD_IDENT, sizeof(MOD_IDENT))) {
                return parse_module_header_v1((u8_t*)file + sections[i].offset);
            }
        }
    }

    return -1;
}

int Module::parse_module_header_v1(void* headerPtr) {

}

void* Module::get_symbol_ptr(const char* name) {
    auto res = f_symbol_map.at(name);
    return res ? (void*)(*res + f_base_addr) : 0;
}

u16_t Module::major() {
    return f_major_num;
}
