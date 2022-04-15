#include <arch/x86_64/apic.h>
#include <arch/x86_64/cpu.h>
#include <arch/x86_64/ports.h>
#include <list.hpp>
#include <locking/locker.hpp>
#include <memory/virtual.hpp>

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
            u64_t entry = interruptVector | (polarity ? (1 << 13) : 0) | (triggerMode ? (1 << 15) : 0) | (mask ? (1 << 16) : 0) | (u64_t)bsp_apic_id << 56;

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

#define MASTER_PIC_COMMAND 0x20
#define MASTER_PIC_DATA 0x21
#define SLAVE_PIC_COMMAND 0xA0
#define SLAVE_PIC_DATA 0xA1
#define HARDWARE_IRQ_BASE 0x20

bool use_apic;
void init_pic() {
    if(ioApics.size() == 0) {
        // Fallback to legacy PICs
        kprintf("[Kernel] Falling back to legacy PIC pair\n");
        // Reinitialize the PICs
        outb(MASTER_PIC_COMMAND, 0x11);
        outb(SLAVE_PIC_COMMAND, 0x11);

        // Set the IRQ base
        outb(MASTER_PIC_DATA, HARDWARE_IRQ_BASE);
        outb(SLAVE_PIC_DATA, HARDWARE_IRQ_BASE + 0x08);

        // Chain the PICs
        outb(MASTER_PIC_DATA, 4);
        outb(SLAVE_PIC_DATA, 2);

        // 8086 mode
        outb(MASTER_PIC_DATA, 0x01);
        outb(SLAVE_PIC_DATA, 0x01);

        // Unmask all IRQs
        outb(MASTER_PIC_DATA, 0x00);
        outb(SLAVE_PIC_DATA, 0x00);

        use_apic = false;
    } else {
        kprintf("[%T] (Kernel) Found %d I/O APIC\n", ioApics.size());

        // Mask the legacy PICs
        outb(MASTER_PIC_DATA, 0xFF);
        outb(SLAVE_PIC_DATA, 0xFF);

        use_apic = true;
    }
}

void pic_eoi(u8_t vector) {
    if(use_apic) {
        write_lapic(0xB0, 0);
    } else {
        if(vector >= HARDWARE_IRQ_BASE && vector < HARDWARE_IRQ_BASE + 16) {
            if(vector >= HARDWARE_IRQ_BASE + 8) outb(SLAVE_PIC_COMMAND, 0x20);
            outb(MASTER_PIC_COMMAND, 0x20);
        }
    }
}
