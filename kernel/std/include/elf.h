#ifndef _MIEROS_KERNEL_ELF_H
#define _MIEROS_KERNEL_ELF_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct Elf64_Header {
    char ident[4];
    u8_t bits;
    u8_t endianess;
    u8_t version;
    u8_t abi;
    u8_t pad[8];
    u16_t type;
    u16_t arch;
    u32_t elf_version;
    u64_t entry_point;
    u64_t phdr_offset;
    u64_t sect_offset;
    u32_t flags;
    u16_t header_size;
    u16_t phdr_entry_size;
    u16_t phdr_entry_count;
    u16_t sect_entry_size;
    u16_t sect_entry_count;
    u16_t sect_name_idx;
};

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_TLS 7
#define PT_LOOS 0x60000000
#define PT_HIOS 0x6FFFFFFF
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7FFFFFFF

struct Elf64_Phdr {
    u32_t type;
    u32_t flags;
    u64_t offset;
    u64_t vaddr;
    u64_t paddr;
    u64_t file_size;
    u64_t mem_size;
    u64_t alignment;
};

#define ST_NULL 0
#define ST_PROGBITS 1
#define ST_SYMTAB 2
#define ST_STRTAG 3
#define ST_RELA 4
#define ST_HASH 5
#define ST_DYNAMIC 6
#define ST_NOTE 7
#define ST_NOBITS 8
#define ST_REL 9
#define ST_SHLIB 10
#define ST_DYNSYM 11
#define ST_INIT_ARRAY 14
#define ST_FINI_ARRAY 15
#define ST_PREINIT_ARRAY 16
#define ST_GROUP 17
#define ST_SYMTAB_SHNDX 18
#define ST_NUM 19
#define ST_LOOS 0x60000000

struct Elf64_Section {
    u32_t name_idx;
    u32_t type;
    u64_t flags;
    u64_t addr;
    u64_t size;
    u32_t link;
    u32_t info;
    u64_t addr_align;
    u64_t entry_size;
};

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ELF_H
