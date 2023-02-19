#include <modules/module.hpp>
#include <elf.h>
#include <dmesg.h>
#include <memory/virtual.hpp>
#include <memory/physical.h>
#include <modules/module_header.h>
#include <modules/module_manager.hpp>

using namespace kernel;

Module::Module(u16_t major)
    : f_major_num(major) {
    f_base_addr = 0;

    f_loaded = false;
    f_initialized = false;
}

Module::~Module() {

}

#define CHECK_ERROR(expr) { err_t _code = (expr); if(_code) { print_error(_code); return ENOEXEC; } }

static const char ELF_IDENT[] = { 0x7F, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static const char MOD_IDENT[] = MODULE_HEADER_MAGIC;
ValueOrError<void> Module::load(void* file) {
    if(f_loaded)
        return { };

    // Do the parsing
    Elf64_Header* elfHeader = (Elf64_Header*)file;
    if(!memcmp(elfHeader, ELF_IDENT, sizeof(ELF_IDENT))) {
        print_error(1);
        return ENOEXEC;
    }

    Elf64_Section* sections = (Elf64_Section*)((u8_t*)file + elfHeader->sect_offset);
    Elf64_Phdr* programHeaders = (Elf64_Phdr*)((u8_t*)file + elfHeader->phdr_offset);
    u8_t* sectionNames = (u8_t*)file + sections[elfHeader->sect_name_idx].offset;

    // First we will calculate how much memory is required to load this module
    virtaddr_t firstAddr = ~0;
    virtaddr_t lastAddr = 0;
    for(size_t i = 0; i < elfHeader->phdr_entry_count; ++i) {
        if(programHeaders[i].type != PT_LOAD) continue;

        virtaddr_t phdrFirst = programHeaders[i].vaddr & ~0xFFF;
        virtaddr_t phdrLast = programHeaders[i].vaddr + programHeaders[i].mem_size;

        if(phdrLast & 0xFFF)
            phdrLast = (phdrLast | 0xFFF) + 1;

        if(firstAddr > phdrFirst) firstAddr = phdrFirst;
        if(lastAddr < phdrLast) lastAddr = phdrLast;
    }

    // Now we find a suitable location
    auto& pager = Pager::kernel();
    size_t modSize = lastAddr - firstAddr;
    size_t modPageSize = (modSize >> 12) + ((modSize & 0xFFF) == 0 ? 0 : 1);

    pager.lock();
    // We need to keep a lock on the pager for our
    // free range to not get used by anything else
    f_base_addr = pager.getFreeRange(KERNEL_START, modPageSize);
    f_page_count = modPageSize;

    if(!f_base_addr) {
        print_error(3);
        pager.unlock();
        return ENOMEM;
    }

    // Let's now save the sections for later use
    for(size_t i = 0; i < elfHeader->sect_entry_count; ++i) {
        // Only save the section if it is allocated
        if(sections[i].flags & SHF_ALLOC) {
            char* name = (char*)(sectionNames + sections[i].name_idx);
            f_section_map.insert({ std::String<>(name), Section {
                std::String<>(name),
                (void*)(sections[i].addr + f_base_addr),
                sections[i].size,
                sections[i].flags,
                sections[i].type,
                sections[i].entry_size
            } });
        }
    }

    struct FlagRegion {
        virtaddr_t start;
        size_t length;
        PageFlags flags;
    };
    std::List<FlagRegion> flagRegions;

    // We now load the module into the memory space we found earlier
    for(size_t i = 0; i < elfHeader->phdr_entry_count; ++i) {
        if(programHeaders[i].type == PT_LOAD) {
            // Load header
            size_t alignedMemSize = programHeaders[i].mem_size + (programHeaders[i].vaddr & 0xFFF);
            size_t pageMemSize = (alignedMemSize >> 12) + ((alignedMemSize & 0xFFF) == 0 ? 0 : 1);

            for(size_t j = 0; j < pageMemSize; ++j)
                pager.map(palloc(1), f_base_addr + (programHeaders[i].vaddr & ~0xFFF) + (j << 12), 1, PageFlags(true, true));

            // Copy from file...
            memcpy((void*)(f_base_addr + programHeaders[i].vaddr), (u8_t*)file + programHeaders[i].offset, programHeaders[i].file_size);

            // ...and clear the rest
            memset((void*)(f_base_addr + programHeaders[i].vaddr + programHeaders[i].file_size), 0, programHeaders[i].mem_size - programHeaders[i].file_size);

            flagRegions.push_back({ f_base_addr + (programHeaders[i].vaddr & ~0xFFF), pageMemSize, PageFlags(
                true, // Present
                programHeaders[i].flags & 2, // Writable
                false, // User
                programHeaders[i].flags & 1, // Executable
                true // Global
            ) });
        }
    }

    pager.unlock();

    // We can parse the module header now that we loaded it into it's location
    CHECK_ERROR(parse_module_header());

    // Get symbols
    CHECK_ERROR(load_symbols());

    // Relocation and linking
    CHECK_ERROR(link());

    u16_t* mod_major_num = (u16_t*)get_symbol_ptr("major");
    if(mod_major_num != 0)
        *mod_major_num = f_major_num;

    // Set correct flags for sections
    pager.lock();
    for(auto& reg : flagRegions) {
        pager.flags(reg.start, reg.length, reg.flags);
    }
    pager.unlock();

    f_loaded = true;
    return { };
}

bool Module::is_loaded() {
    return f_loaded;
}

int Module::parse_module_header() {
    auto sectionOpt = f_section_map.at(".modulehdr");
    if(!sectionOpt) return 2;

    int status = parse_module_header_v1(sectionOpt->f_ptr, sectionOpt->f_size);
    if(!status) return 0;

    // Later on we will check for status code 5 (header not found) and
    // when it occurs parse headers of different versions
    print_error(status);
    return status;
}

int Module::parse_module_header_v1(void* headerPtr, size_t size) {
    module_header* header = (module_header*)memfind(headerPtr, MOD_IDENT, sizeof(MOD_IDENT), size);
    if(header == 0) return 5;

    f_name = header->mod_name;

    if(header->dependencies) {
        // This is happening before relocations so we need to add base address to things
        for(char** ptr = (char**)((u8_t*)header->dependencies + f_base_addr); *ptr != 0; ++ptr) {
            char* depName = (char*)((u8_t*)*ptr + f_base_addr);
            u16_t major_num = ModuleManager::get().find_module(depName);
            f_dependencies.push_back(major_num);
        }
    }

    return 0;
}

const std::String<>& Module::name() const {
    return f_name;
}

int Module::load_symbols() {
    auto symSecOpt = f_section_map.at(".dynsym");
    auto symStrOpt = f_section_map.at(".dynstr");

    if(!symSecOpt || !symStrOpt) {
        dmesg("(Kernel) Module does not contain a symbol section");
        return 1;
    }

    f_symbols = (Elf64_Symbol*)symSecOpt->f_ptr;
    f_symbol_names = (char*)symStrOpt->f_ptr;
    f_symbol_count = symSecOpt->f_size / symSecOpt->f_entry_size;

    return 0;
}

Elf64_Symbol* Module::get_symbol(size_t index) {
    if(index >= f_symbol_count) return 0;
    return f_symbols + index;
}

Elf64_Symbol* Module::get_symbol(const char* name) {
    for(size_t i = 0; i < f_symbol_count; ++i) {
        if(!strcmp(f_symbol_names + f_symbols[i].name_idx, name)) {
            return f_symbols + i;
        }
    }
    return 0;
}

void* Module::get_symbol_ptr(const char* name) {
    auto* symbol = get_symbol(name);
    return symbol ? (void*)(symbol->addr + f_base_addr) : 0;
}

u16_t Module::major() {
    return f_major_num;
}

void Module::print_error(int errCode) {
    switch(errCode) {
        case 0:
            break;
        case 1:
            dmesg("(Kernel) Invalid module format");
            break;
        case 2:
            dmesg("(Kernel) Module is missing a module header");
            break;
        case 3:
            dmesg("(Kernel) Failed to allocate space for the module");
            break;
        case 4:
            dmesg("(Kernel) Module linking failed, symbol does not exist");
            break;
        case 5:
            dmesg("(Kernel) Module header not found");
            break;
        case 6:
            dmesg("(Kernel) Undefined symbol in module");
            break;
        default:
            dmesg("(Kernel) Unknown module error (code %d)", errCode);
            break;
    }
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

int Module::link() {
    for(auto [name, section] : f_section_map) {
        if(section.f_type == ST_RELA) {
            Elf64_Rela* rela = (Elf64_Rela*)section.f_ptr;
            for(size_t i = 0; i < section.f_size / section.f_entry_size; ++i) {
                switch(ELF64_REL_TYPE(rela[i].info)) {
                    case 0x06: { // R_x86_64_GLOB_DATA
                        auto* symbol = get_symbol(ELF64_REL_SYM(rela[i].info));
                        if(symbol == 0)
                            return 4;

                        if(symbol->addr != 0) {
                            *((u64_t*)(f_base_addr + rela[i].addr)) = symbol->addr;
                        } else {
                            // Look for the symbol in kernel symbols
                            char* sym_name = f_symbol_names + symbol->name_idx;
                            auto* kern_sym = find_kernel_symbol(sym_name);
                            if(kern_sym != 0) {
                                *(u64_t*)(f_base_addr + rela[i].addr) = kern_sym->addr;
                                break;
                            }

                            // Look for the symbol in dependencies of the module
                            bool found = false;
                            for(u16_t major : f_dependencies) {
                                Module* mod = ModuleManager::get().get_module(major);
                                auto* symbol_ptr = mod->get_symbol_ptr(sym_name);
                                if(symbol_ptr != 0) {
                                    *(void**)(f_base_addr + rela[i].addr) = symbol_ptr;
                                    found = true;
                                    break;
                                }
                            }

                            if(!found)
                                return 6;
                        }
                        break;
                    } case 0x08: { // R_x86_64_RELATIVE
                        *(u64_t*)(f_base_addr + rela[i].addr) = f_base_addr + rela[i].addend;
                        break;
                    } default:
                        dmesg("(Kernel) \033[31;1mError!\033[0m Unknown relocation type %d", ELF64_REL_TYPE(rela[i].info));
                        break;
                }
            }
        }
    }

    return 0;
}

void Module::constructors() {
    auto ctorOpt = f_section_map.at(".ctors");
    if(ctorOpt) {
        for(size_t i = 0; i < ctorOpt->f_size; i += 8) {
            u64_t constructor_addr = *(u64_t*)((u8_t*)ctorOpt->f_ptr + i);
            ((void (*)())(constructor_addr))();
        }
    }

    auto initArrayOpt = f_section_map.at(".init_array");
    if(initArrayOpt) {
        for(size_t i = 0; i < initArrayOpt->f_size; i += initArrayOpt->f_entry_size) {
            u64_t constructor_addr = *(u64_t*)((u8_t*)initArrayOpt->f_ptr + i);
            ((void (*)())(constructor_addr))();
        }
    }
}

void Module::destructors() {
    auto dtorOpt = f_section_map.at(".dtors");
    if(dtorOpt) {
        for(size_t i = 0; i < dtorOpt->f_size; i += 8) {
            u64_t constructor_addr = *(u64_t*)((u8_t*)dtorOpt->f_ptr + i);
            ((void (*)())(constructor_addr))();
        }
    }
}

bool Module::is_initialized() {
    return f_initialized;
}

int Module::preinit() {
    constructors();

    // Initialize all dependencies
    for(auto& dep : f_dependencies) {
        auto* mod = ModuleManager::get().get_module(dep);
        if(!mod)
            return -1;
        if(!mod->is_initialized())
            mod->init();
    }

    return 0;
}
