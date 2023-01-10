#include <dmesg.h>
#include <memory/physical.h>
#include <memory/virtual.hpp>
#include <modules/module.hpp>
#include <modules/module_header.h>
#include <stdlib.h>
#include <tasking/thread.hpp>

using namespace kernel;

const u8_t ELF_HEADER[] = { 0x7F, 'E', 'L', 'F', 2, 1, 1, 0 };
Module::Module(void* elf_file, u16_t major_num)
    : major_num(major_num) {
    u8_t* elf_file_c = (u8_t*)elf_file;
    Elf64_Header* elf_header = (Elf64_Header*)elf_file;
    if(!memcmp(elf_header, ELF_HEADER, 8)) {
        dmesg("(Kernel) \033[31;1mError!\033[0m Invalid module ELF header!");
        return;
    }

    Elf64_Phdr* program_headers = (Elf64_Phdr*)(elf_file_c + elf_header->phdr_offset);

    // Determine how much address space is required
    virtaddr_t start_addr = ~0;
    virtaddr_t end_addr = 0;

    for(int i = 0; i < elf_header->phdr_entry_count; ++i) {
        if(program_headers[i].type == PT_LOAD) {
            if(start_addr > program_headers[i].vaddr) start_addr = program_headers[i].vaddr;
            if(end_addr < program_headers[i].vaddr + program_headers[i].mem_size) end_addr = program_headers[i].vaddr + program_headers[i].mem_size;
        }
    }
    // Align the end address
    end_addr = (end_addr & ~0xFFF) + ((end_addr & 0xFFF) == 0 ? 0 : 0x1000);

    Pager::kernel().lock();
    address_base = Pager::kernel().getFreeRange(KERNEL_START, (end_addr - start_addr) >> 12);

    struct ReflagEntry {
        virtaddr_t address;
        size_t length;
        PageFlags flags;
    };
    std::List<ReflagEntry> reflag_range;

    for(int i = 0; i < elf_header->phdr_entry_count; ++i) {
        if(program_headers[i].type == PT_LOAD) {
            size_t page_size = (program_headers[i].mem_size >> 12) + ((program_headers[i].mem_size & 0xFFF) == 0 ? 0 : 1);
            bool executable = program_headers[i].flags & 1;
            bool writable = program_headers[i].flags & 2;
            for(size_t j = 0; j < page_size; ++j)
                Pager::kernel().map(palloc(1), address_base + program_headers[i].vaddr + (j << 12), 1, { 1, 1, 0, executable, 1, 0 });
            memcpy((void*)(address_base + program_headers[i].vaddr), elf_file_c + program_headers[i].offset, program_headers[i].file_size);
            allocated_ranges.add(program_headers[i].vaddr, program_headers[i].vaddr + (page_size << 12));
        
            if(!writable) reflag_range.push_back({ address_base + program_headers[i].vaddr, page_size, { 1, 0, 0, executable, 1, 0 } });
        }
    }

    Pager::kernel().unlock();

    Elf64_Section* sections_file = (Elf64_Section*)(elf_file_c + elf_header->sect_offset);
    char* names = (char*)(elf_file_c + sections_file[elf_header->sect_name_idx].offset);
    for(int i = 0; i < elf_header->sect_entry_count; ++i) {
        Elf64_Section& section = sections_file[i];
        if(section.flags & SHF_ALLOC) {
            sections.push_back({ names + section.name_idx,
                section.addr + address_base,
                section.size,
                section.type,
                (u32_t)section.flags,
                (u32_t)section.entry_size });
        }
    }

    auto symtab = get_section(".dynsym");
    auto symstr = get_section(".dynstr");
    if(symtab)
        symbol_table = &*symtab;
    else {
        dmesg("(Kernel) \033[31;1mError!\033[0m Module does not have a dynamic symbol table");
        return;
    }
    if(symstr)
        symbol_names = &*symstr;
    else {
        dmesg("(Kernel) \033[31;1mError!\033[0m Module does not have a dynamic symbol name table");
        return;
    }

    auto header_sec = get_section(".modulehdr");
    if(!header_sec) {
        dmesg("(Kernel) \033[31;1mError!\033[0m Module does not have a header section");
        return;
    }

    link();

    for(auto& entry : reflag_range) {
        Pager::kernel().lock();
        Pager::kernel().flags(entry.address, entry.length, entry.flags);
        Pager::kernel().unlock();
    }

    // Ready for execution

    module_header* header = (module_header*)header_sec->address;
    init_signals = (char*)header->init_on_ptr;
    f_flags = header->flags;

    run_ctors();

    initialized = false;
}

Module::~Module() {
    if(initialized) {
        run_function<void>("destroy");
        initialized = false;
    }
    run_dtors();
}

bool Module::is_appropriate(const char* init_signal) {
    char* signals = init_signals;
    while(signals[0] != 0) {
        if(strmatch(signals, init_signal)) return true;
        signals += strlen(signals) + 1;
    }
    return false;
}

extern "C" Elf64_Symbol _dynsym_start;
extern "C" Elf64_Symbol _dynsym_end;
extern "C" char _dynstr_start;
Elf64_Symbol* find_kernel_symbol(const char* name) {
    for(Elf64_Symbol* symbol = &_dynsym_start; symbol != &_dynsym_end; ++symbol) {
        char* sym_name = &_dynstr_start + symbol->name_idx;
        if(!strcmp(name, sym_name)) return symbol;
    }
    return 0;
}

void Module::link() {
    for(auto& section : sections) {
        if(section.type == ST_RELA) {
            for(size_t i = 0; i < section.size; i += section.entry_size) {
                Elf64_Rela* rela = (Elf64_Rela*)(section.address + i);
                switch(ELF64_REL_TYPE(rela->info)) {
                    case 0x06: { // R_x86_64_GLOB_DATA
                        auto symbol = get_symbol(ELF64_REL_SYM(rela->info));
                        if(!symbol) dmesg("(Kernel) \033[31;1mError!\033[0m Could not find symbol referenced in relocation section in the .dynsym section");
                        if(symbol->addr != 0)
                            *(u64_t*)(address_base + rela->addr) = symbol->addr;
                        else {
                            // Start looking for the symbol elsewhere
                            char* sym_name = (char*)(symbol_names->address + symbol->name_idx);
                            auto* kern_sym = find_kernel_symbol(sym_name);
                            if(kern_sym != 0) {
                                *(u64_t*)(address_base + rela->addr) = kern_sym->addr;
                                break;
                            }

                            /// TODO: [29.01.2022] Look in the dependencies of this module.
                            dmesg("(Kernel) TODO: Look in the dependencies of this module.");
                        }
                        break;
                    }
                    case 0x08: { // R_x86_64_RELATIVE
                        *(u64_t*)(address_base + rela->addr) = address_base + rela->addend;
                        break;
                    }
                    default:
                        kprintf("[%T] (Kernel) \033[31;1mError!\033[0m Unknown relocation type %d\n", ELF64_REL_TYPE(rela->info));
                        break;
                }
            }
        }
    }
}

int Module::init(void* init_struct) {
    if(initialized) return 0;
    initialized = true;
    if(init_struct == 0)
        return run_function<int>("init");
    else
        return run_function<int>("init", init_struct);
}

void Module::run_ctors() {
    auto section = get_section(".ctors");
    if(section) {
        Module* old = Thread::current()->f_current_module;
        Thread::current()->f_current_module = this;
        for(size_t i = 0; i < section->size; i += 8) {
            u64_t constructor_addr = *(u64_t*)(section->address + i);
            ((void (*)())(constructor_addr))();
        }
        Thread::current()->f_current_module = old;
    }

    auto section2 = get_section(".init_array");
    if(section2) {
        Module* old = Thread::current()->f_current_module;
        Thread::current()->f_current_module = this;
        for(size_t i = 0; i < section2->size; i += 8) {
            u64_t constructor_addr = *(u64_t*)(section2->address + i);
            if(constructor_addr == 0 || constructor_addr == 0xFFFFFFFFFFFFFFFF) continue;
            ((void (*)())(constructor_addr))();
        }
        Thread::current()->f_current_module = old;
    }
}

void Module::run_dtors() {
    auto section = get_section(".dtors");
    if(section) {
        Module* old = Thread::current()->f_current_module;
        Thread::current()->f_current_module = this;
        for(size_t i = 0; i < section->size; i += 8) {
            u64_t constructor_addr = *(u64_t*)(section->address + i);
            ((void (*)())(constructor_addr))();
        }
        Thread::current()->f_current_module = old;
    }
}

std::OptionalRef<Module::Section> Module::get_section(const char* name) {
    for(auto& section : sections) {
        if(!strcmp(section.name.c_str(), name)) return section;
    }
    return {};
}

std::OptionalRef<Elf64_Symbol> Module::get_symbol(const char* name) {
    for(size_t i = 0; i < symbol_table->size; i += symbol_table->entry_size) {
        Elf64_Symbol* symbol = (Elf64_Symbol*)(symbol_table->address + i);
        char* sym_name = (char*)(symbol_names->address + symbol->name_idx);
        if(!strcmp(sym_name, name)) return *symbol;
    }
    return {};
}

std::OptionalRef<Elf64_Symbol> Module::get_symbol(size_t index) {
    size_t offset = symbol_table->entry_size * index;
    if(offset >= symbol_table->size) return {};
    return *(Elf64_Symbol*)(symbol_table->address + offset);
}

Module::ThreadModuleSetter::ThreadModuleSetter(Module* mod) {
    Thread* thread = Thread::current();
    old_mod = thread->f_current_module;
    thread->f_current_module = mod;
}

Module::ThreadModuleSetter::~ThreadModuleSetter() {
    Thread::current()->f_current_module = old_mod;
}
