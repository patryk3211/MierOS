#include <arch/x86_64/apic.h>
#include <list.hpp>
#include <memory/virtual.hpp>
#include <locking/locker.hpp>

using namespace kernel;

struct APIC {
    u8_t id;
    u8_t maxEntries;
    physaddr_t address;
    u32_t gsiBase;
    u32_t usedPins;
    virtaddr_t mapped;
};

void map_ioapic(APIC& apic) {
    Pager& pager = Pager::active();
    Locker lock(pager);

    apic.mapped = pager.kmap(apic.address, 1, { 1, 1, 0, 0, 1, 0 });
}

void unmap_ioapic(APIC& apic) {
    Pager& pager = Pager::active();
    Locker lock(pager);

    pager.unmap(apic.mapped, 1);
    apic.mapped = 0;
}

u32_t read_ioapic(APIC& apic, u32_t reg) {
    volatile u32_t* mem = (volatile u32_t*)apic.mapped;
    mem[0] = reg;
    return mem[4];
}

void write_ioapic(APIC& apic, u32_t reg, u32_t value) {
    volatile u32_t* mem = (volatile u32_t*)apic.mapped;
    mem[0] = reg;
    mem[4] = value;
}

std::List<APIC> ioApics = std::List<APIC>();

void add_ioapic(u8_t id, u32_t address, u32_t globalSystemInterrupBase) {
    APIC apic = {
        .id = id,
        .maxEntries = 0,
        .address = address,
        .gsiBase = globalSystemInterrupBase,
        .usedPins = 0,
        .mapped = 0
    };

    map_ioapic(apic);
    u32_t version = read_ioapic(apic, 0x01);
    unmap_ioapic(apic);

    apic.maxEntries = ((version >> 16) & 0xFF) + 1;

    ioApics.push_back(apic);
}

extern u32_t bsp_apic_id;

void add_ioapic_intentry(u8_t interruptVector, u32_t globalSystemInterrupt, u8_t triggerMode, u8_t polarity, u8_t mask) {
    for(auto& apic : ioApics) {
        if(globalSystemInterrupt >= apic.gsiBase && globalSystemInterrupt < apic.gsiBase + apic.maxEntries) {
            u64_t entry =
                interruptVector |
                (polarity ? (1 << 13) : 0) |
                (triggerMode ? (1 << 15) : 0) |
                (mask ? (1 << 16) : 0) |
                (u64_t)bsp_apic_id << 56;

            u32_t entryOffset = (globalSystemInterrupt - apic.gsiBase);

            map_ioapic(apic);
            write_ioapic(apic, 0x10 + entryOffset * 2, entry);
            write_ioapic(apic, 0x11 + entryOffset * 2, entry >> 32);
            unmap_ioapic(apic);

            apic.usedPins |= 1 << entryOffset;

            return;
        }
    }
}
