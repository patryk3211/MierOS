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

#define SHF_WRITE (1 << 0)
#define SHF_ALLOC (1 << 1)
#define SHF_EXECINSTR (1 << 2)
#define SHF_MERGE (1 << 4)
#define SHF_STRINGS (1 << 5)
#define SHF_INFO_LINK (1 << 6)
#define SHF_LINK_ORDER (1 << 7)
#define SHF_OS_NONCONFORMING (1 << 8)
#define SHF_GROUP (1 << 9)
#define SHF_TLS (1 << 10)
#define SHF_COMPRESSED (1 << 11)

struct Elf64_Section {
    u32_t name_idx;
    u32_t type;
    u64_t flags;
    u64_t addr;
    u64_t offset;
    u64_t size;
    u32_t link;
    u32_t info;
    u64_t addr_align;
    u64_t entry_size;
};

#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_COMMON 5
#define STT_TLS 6
#define STT_NUM 7

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_NUM 3

#define ELF64_ST_BIND(val) (((u8_t)(val)) >> 4)
#define ELF64_ST_TYPE(val) ((val)&0xf)
#define ELF64_ST_INFO(bind, type) (((bind) << 4) + ((type)&0xf))

struct Elf64_Symbol {
    u32_t name_idx;
    u8_t info;
    u8_t other;
    u16_t sec_idx;
    u64_t addr;
    u64_t size;
};

struct Elf64_Rela {
    u64_t addr;
    u64_t info;
    u64_t addend;
};

#define ELF64_REL_SYM(i) ((i) >> 32)
#define ELF64_REL_TYPE(i) ((i)&0xffffffff)

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ELF_H
