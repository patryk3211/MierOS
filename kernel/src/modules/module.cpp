#include <modules/module.hpp>
#include <stdlib.h>
#include <dmesg.h>
#include <memory/virtual.hpp>
#include <memory/physical.h>
#include <tasking/thread.hpp>

using namespace kernel;

const u8_t ELF_HEADER[] = { 0x7F, 'E', 'L', 'F', 2, 1, 1, 0 };
Module::Module(void* elf_file, u16_t major_num) : major_num(major_num) {
    u8_t* elf_file_c = (u8_t*)elf_file;
    Elf64_Header* elf_header = (Elf64_Header*)elf_file;
    if(!memcmp(elf_header, ELF_HEADER, 8)) {
        dmesg("[Kernel] \033[31;1mError!\033[0m Invalid module ELF header!\n");
        return;
    }

    Elf64_Phdr* program_headers = (Elf64_Phdr*)(elf_file_c+elf_header->phdr_offset);

    // Determine how much address space is required
    virtaddr_t start_addr = ~0;
    virtaddr_t end_addr = 0;

    for(int i = 0; i < elf_header->phdr_entry_count; ++i) {
        if(program_headers[i].type == PT_LOAD) {
            if(start_addr > program_headers[i].vaddr) start_addr = program_headers[i].vaddr;
            if(end_addr < program_headers[i].vaddr+program_headers[i].mem_size) end_addr = program_headers[i].vaddr+program_headers[i].mem_size;
        }
    }
    // Align the end address
    end_addr = (end_addr & ~0xFFF) + ((end_addr & 0xFFF) == 0 ? 0 : 0x1000);

    Pager::kernel().lock();
    address_base = Pager::kernel().getFreeRange(KERNEL_START, (end_addr - start_addr) >> 12);

    for(int i = 0; i < elf_header->phdr_entry_count; ++i) {
        if(program_headers[i].type == PT_LOAD) {
            size_t page_size = (program_headers[i].mem_size >> 12) + ((program_headers[i].mem_size & 0xFFF) == 0 ? 0 : 1);
            bool executable = program_headers[i].flags & 1;
            bool writable = program_headers[i].flags & 2;
            for(size_t j = 0; j < page_size; ++j)
                Pager::kernel().map(palloc(1), address_base + program_headers[i].vaddr + (j << 12), 1, { .present = 1, .writable = writable, .user_accesible = 0, .executable = executable, .global = 1 });
            memcpy((void*)(address_base + program_headers[i].vaddr), elf_file_c+program_headers[i].offset, program_headers[i].file_size);
            allocated_ranges.add(program_headers[i].vaddr, program_headers[i].vaddr+(page_size<<12));
        }
    }

    Pager::kernel().unlock();

    Elf64_Section* sections_file = (Elf64_Section*)(elf_file_c+elf_header->sect_offset);
    char* names = (char*)(elf_file_c+sections_file[elf_header->sect_name_idx].offset);
    for(int i = 0; i < elf_header->sect_entry_count; ++i) {
        Elf64_Section& section = sections_file[i];
        if(section.flags & SHF_ALLOC) {
            sections.push_back({
                names+section.name_idx,
                section.addr+address_base,
                section.size,
                section.type,
                (u32_t)section.flags,
                (u32_t)section.entry_size
            });
        }
    }

    auto symtab = get_section(".dynsym");
    auto symstr = get_section(".dynstr");
    if(symtab) symbol_table = &*symtab;
    else {
        dmesg("[Kernel] \033[31;1mError!\033[0m Module does not have a dynamic symbol table\n");
        return;
    }
    if(symstr) symbol_names = &*symstr;
    else {
        dmesg("[Kernel] \033[31;1mError!\033[0m Module does not have a dynamic symbol name table\n");
        return;
    }

    link();
    run_ctors();
}

Module::~Module() {
    run_dtors();
}

extern "C" Elf64_Symbol _dynsym_start;
extern "C" Elf64_Symbol _dynsym_end;
extern "C" char _dynstr_start;
Elf64_Symbol* find_kernel_symbol(std::String<> name) {
    for(Elf64_Symbol* symbol = &_dynsym_start; symbol != &_dynsym_end; ++symbol) {
        char* sym_name = &_dynstr_start+symbol->name_idx;
        if(name == sym_name) return symbol;
    }
    return 0;
}

void Module::link() {
    for(auto& section : sections) {
        if(section.type == ST_RELA) {
            for(size_t i = 0; i < section.size; i += section.entry_size) {
                Elf64_Rela* rela = (Elf64_Rela*)(section.address+i);
                switch(ELF64_REL_TYPE(rela->info)) {
                    case 0x06: { // R_x86_64_GLOB_DATA
                        auto symbol = get_symbol(ELF64_REL_SYM(rela->info));
                        if(!symbol) dmesg("[Kernel] \033[31;1mError!\033[0m Could not find symbol referenced in relocation section in the .dynsym section\n");
                        if(symbol->addr != 0) *(u64_t*)(address_base+rela->addr) = symbol->addr;
                        else {
                            // Start looking for the symbol elsewhere
                            char* sym_name = (char*)(symbol_names->address+symbol->name_idx);
                            auto* kern_sym = find_kernel_symbol(sym_name);
                            if(kern_sym != 0) {
                                *(u64_t*)(address_base+rela->addr) = kern_sym->addr;
                                break;
                            }

                            /// TODO: [29.01.2022] Look in the dependencies of this module.
                            dmesg("[Kernel] TODO: Look in the dependencies of this module.\n");
                        }
                        break;
                    } default:
                        kprintf("[Kernel] \033[31;1mError!\033[0m Unknown relocation type %d\n", ELF64_REL_TYPE(rela->info));
                        break;
                }
            }
        }
    }
}

void Module::run_ctors() {
    auto section = get_section(".ctors");
    if(section) {
        for(size_t i = 0; i < section->size; i += 8) {
            u64_t constructor_addr = *(u64_t*)(section->address+i);
            ((void (*)())(constructor_addr+address_base))();
        }
    }
}

void Module::run_dtors() {
    auto section = get_section(".dtors");
    if(section) {
        for(size_t i = 0; i < section->size; i += 8) {
            u64_t constructor_addr = *(u64_t*)(section->address+i);
            ((void (*)())(constructor_addr+address_base))();
        }
    }
}

std::OptionalRef<Module::Section> Module::get_section(std::String<> name) {
    for(auto& section : sections) {
        if(section.name == name) return section;
    }
    return {};
}

std::OptionalRef<Elf64_Symbol> Module::get_symbol(std::String<> name) {
    for(size_t i = 0; i < symbol_table->size; i += symbol_table->entry_size) {
        Elf64_Symbol* symbol = (Elf64_Symbol*)(symbol_table->address+i);
        char* sym_name = (char*)(symbol_names->address+symbol->name_idx);
        if(name == sym_name) return *symbol;
    }
    return {};
}

std::OptionalRef<Elf64_Symbol> Module::get_symbol(size_t index) {
    size_t offset = symbol_table->entry_size*index;
    if(offset >= symbol_table->size) return {};
    return *(Elf64_Symbol*)(symbol_table->address+offset);
}

Module::ThreadModuleSetter::ThreadModuleSetter(Module* mod) {
    Thread* thread = Thread::current();
    old_mod = thread->current_module;
    thread->current_module = mod;
}

Module::ThreadModuleSetter::~ThreadModuleSetter() {
    Thread::current()->current_module = old_mod;
}
