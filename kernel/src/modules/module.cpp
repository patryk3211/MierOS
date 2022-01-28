#include <modules/module.hpp>
#include <stdlib.h>
#include <dmesg.h>
#include <memory/virtual.hpp>
#include <memory/physical.h>

using namespace kernel;

const u8_t ELF_HEADER[] = { 0x7F, 'E', 'L', 'F', 2, 1, 1, 0 };
Module::Module(void* elf_file) {
    u8_t* elf_file_c = (u8_t*)elf_file;
    Elf64_Header* elf_header = (Elf64_Header*)elf_file;
    if(!memcmp(elf_header, ELF_HEADER, 8)) {
        dmesg("[Kernel] Invalid module ELF header!\n");
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
                Pager::kernel().map(palloc(1), program_headers[i].vaddr + (j << 12), 1, { .present = 1, .writable = writable, .user_accesible = 0, .executable = executable, .global = 1 });
            memcpy((void*)program_headers[i].vaddr, elf_file_c+program_headers[i].offset, program_headers[i].file_size);
        }
    }

    Pager::kernel().unlock();
}

Module::~Module() {

}
