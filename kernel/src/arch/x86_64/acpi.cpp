#include <arch/x86_64/acpi.h>
#include <memory/virtual.hpp>
#include <locking/locker.hpp>
#include <sections.h>
#include <arch/cpu.h>
#include <unordered_map.hpp>
#include <dmesg.h>
#include <string.hpp>

using namespace kernel;

struct ACPI_RSDP {
    char sign[8];
    u8_t checksum;
    char oemId[6];
    u8_t revision;
    u32_t rsdtAddress;
}__attribute__((packed));

struct ACPI_RSDP2 {
    ACPI_RSDP rsdp;

    u32_t length;
    u64_t xsdtAddress;
    u8_t checksum;
    u8_t reserved[3];
}__attribute__((packed));

struct ACPI_RSDT {
    ACPI_SDTHeader header;
    u32_t other[0];
}__attribute__((packed));

struct ACPI_XSDT {
    ACPI_SDTHeader header;
    u64_t other[0];
}__attribute__((packed));

std::UnorderedMap<std::String<>, physaddr_t> acpi_tables = std::UnorderedMap<std::String<>, physaddr_t>();

extern "C" TEXT_FREE_AFTER_INIT void init_acpi(physaddr_t rsdp) {
    Pager& pager = Pager::active();
    Locker locker(pager);

    virtaddr_t mapped_addr = pager.kmap(rsdp, 2, { 1, 0, 0, 0, 0 });
    
    u8_t check1 = 0;
    for(size_t i = 0; i < sizeof(ACPI_RSDP); ++i) check1 += ((u8_t*)mapped_addr)[i];
    u8_t check2 = 0;
    for(size_t i = 0; i < sizeof(ACPI_RSDP2); ++i) check2 += ((u8_t*)mapped_addr)[i];

    if(check2 == 0) {
        // XSDT found.
        dmesg("[Kernel] Found a valid ACPI XSDT\n");
        physaddr_t xsdt_addr = ((ACPI_RSDP2*)mapped_addr)->xsdtAddress;
        ACPI_XSDT* xsdt = (ACPI_XSDT*)pager.kmap(xsdt_addr, 2, { 1, 0, 0, 0, 0 });
        
        size_t byte_size = xsdt->header.length + (xsdt_addr & 0xFFF);
        size_t page_size = (byte_size >> 12) + ((byte_size & 0xFFF) == 0 ? 0 : 1);
        pager.unmap((virtaddr_t)xsdt, 2);
        xsdt = (ACPI_XSDT*)pager.kmap(xsdt_addr, page_size, { 1, 0, 0, 0, 0 });

        size_t entries = (xsdt->header.length - sizeof(xsdt->header)) / 8;
        for(size_t i = 0; i < entries; ++i) {
            u64_t addr = xsdt->other[i];
            
            ACPI_SDTHeader* header = (ACPI_SDTHeader*)pager.kmap(addr, 1, { 1, 0, 0, 0, 0 });
            char sign[5];
            memcpy(sign, header->sign, 4);
            sign[4] = 0;
            acpi_tables.insert({ sign, addr });
            pager.unmap((virtaddr_t)header, 1);

            kprintf("[Kernel] ACPI Table (Signature '%s') found at 0x%x16\n", sign, addr);
        }

        pager.unmap((virtaddr_t)xsdt, page_size);
    } else if(check1 == 0) {
        // RSDT found.
        dmesg("[Kernel] Found a valid ACPI RSDT\n");
        physaddr_t rsdt_addr = ((ACPI_RSDP*)mapped_addr)->rsdtAddress;
        ACPI_RSDT* rsdt = (ACPI_RSDT*)pager.kmap(rsdt_addr, 2, { 1, 0, 0, 0, 0 });
        
        size_t byte_size = rsdt->header.length + (rsdt_addr & 0xFFF);
        size_t page_size = (byte_size >> 12) + ((byte_size & 0xFFF) == 0 ? 0 : 1);
        pager.unmap((virtaddr_t)rsdt, 2);
        rsdt = (ACPI_RSDT*)pager.kmap(rsdt_addr, page_size, { 1, 0, 0, 0, 0 });

        size_t entries = (rsdt->header.length - sizeof(rsdt->header)) / 4;
        for(size_t i = 0; i < entries; ++i) {
            u32_t addr = rsdt->other[i];
            
            ACPI_SDTHeader* header = (ACPI_SDTHeader*)pager.kmap(addr, 1, { 1, 0, 0, 0, 0 });
            char sign[5];
            memcpy(sign, header->sign, 4);
            sign[4] = 0;
            acpi_tables.insert({ sign, addr });
            pager.unmap((virtaddr_t)header, 1);

            kprintf("[Kernel] ACPI Table (Signature '%s') found at 0x%x8\n", sign, addr);
        }

        pager.unmap((virtaddr_t)rsdt, page_size);
    } else {
        // No valid table has been found.
        panic("No valid ACPI Table Found!");
    }

    pager.unmap(mapped_addr, 2);
}

extern "C" physaddr_t get_table(const char* sign) {
    auto value = acpi_tables.at(sign);
    if(value) return *value;
    else return 0;
}