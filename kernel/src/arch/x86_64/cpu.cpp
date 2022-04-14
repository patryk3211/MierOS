#include <arch/cpu.h>
#include <arch/x86_64/acpi.h>
#include <memory/virtual.hpp>
#include <locking/locker.hpp>
#include <unordered_map.hpp>
#include <arch/x86_64/cpu.h>
#include <arch/interrupts.h>
#include <memory/physical.h>
#include <tasking/scheduler.hpp>
#include <defines.h>
#include <arch/x86_64/ports.h>
#include <arch/x86_64/apic.h>

using namespace kernel;

struct MADT_Record {
    u8_t type;
    u8_t length;

    u8_t rest[];
} PACKED;

struct ACPI_MADT {
    ACPI_SDTHeader header;

    u32_t lapic_addr;
    u32_t flags;

    u8_t records[];
} PACKED;

// LAPIC Id - Linear Id
NO_EXPORT std::UnorderedMap<u32_t, u32_t> core_map(16);

#define LAPIC_MSR 0x1B

#define LAPIC_VIRTUAL_ADDRESS 0xFFFFFFFFFFDFF000

extern "C" void write_lapic(u32_t offset, u32_t value) {
    *((volatile u32_t*)(LAPIC_VIRTUAL_ADDRESS+offset)) = value;
}

extern "C" u32_t read_lapic(u32_t offset) {
    return *((volatile u32_t*)(LAPIC_VIRTUAL_ADDRESS+offset));
}

void send_init_deassert(u8_t destination) {
    write_lapic(0x310, destination << 24);
    write_lapic(0x300, (0x5 << 8) | (1 << 15));
    while(read_lapic(0x300) & (1 << 12));
}

void send_ipi(u8_t vector, u8_t mode, u8_t destination) {
    write_lapic(0x310, destination << 24);
    write_lapic(0x300, vector | ((mode & 7) << 8) | (1 << 14));
    while(read_lapic(0x300) & (1 << 12)) {
        u32_t err = read_lapic(0x280);
        if(err != 0) kprintf("%x8\n", err);
    }
}

TEXT_FREE_AFTER_INIT void enable_lapic() {
    wrmsr(LAPIC_MSR, rdmsr(LAPIC_MSR) | 0x800);
    write_lapic(0xF0, read_lapic(0xF0) | 0x1FF);
}

u32_t bsp_apic_id;

TEXT_FREE_AFTER_INIT void parse_madt() {
    auto& pager = Pager::active();
    Locker locker(pager);

    auto madt_addr = get_table("APIC");
    auto* madt = (ACPI_MADT*)pager.kmap(madt_addr, 2, { 1, 0, 0, 0, 0, 0 });

    size_t byte_size = (madt_addr & 0xFFF) + madt->header.length;
    size_t page_size = (byte_size >> 12) + ((byte_size & 0xFFF) == 0 ? 0 : 1);

    pager.unmap((virtaddr_t)madt, 2);
    madt = (ACPI_MADT*)pager.kmap(madt_addr, page_size, { 1, 0, 0, 0, 0, 0 });
    physaddr_t lapic_addr = madt->lapic_addr;

    { // Get the address override before anything else
        size_t record_length = madt->header.length - sizeof(madt->header);
        for(size_t offset = 0; offset < record_length; offset += *(madt->records+offset+1)) {
            MADT_Record* record = (MADT_Record*)(madt->records+offset);
            if(record->type == 0x05) {
                // Address override.
                lapic_addr = *((u64_t*)record->rest+2);
                break;
            }
        }
    }

    // Map Local APIC to a constant address
    pager.map(lapic_addr, LAPIC_VIRTUAL_ADDRESS, 1, { 1, 1, 0, 0, 1, 0 });
    enable_lapic();

    // BSP is always guaranteed to be core 0
    bsp_apic_id = read_lapic(0x20) >> 24;
    core_map.insert({ bsp_apic_id, 0 });

    // Parse the rest of the records
    size_t record_length = madt->header.length - sizeof(madt->header);
    for(size_t offset = 0; offset < record_length; offset += *(madt->records+offset+1)) {
        MADT_Record* record = (MADT_Record*)(madt->records+offset);
        switch(record->type) {
            case 0x00: { // Local APIC Entry
                u8_t acpi_id = record->rest[0];
                u8_t apic_id = record->rest[1];
                u8_t flags = record->rest[2];
                kprintf("[Kernel] LAPIC ACPI_ID=%x2 APIC_ID=%x2 FLAGS=%x2\n", acpi_id, apic_id, flags);
                if(flags & 0x03) {
                    // This CPU can be enabled so we put it into our core map.
                    core_map.insert({ apic_id, static_cast<u32_t>(core_map.size()) });
                }
                break;
            } case 0x01: { // IO APIC Entry
                u8_t apic_id = record->rest[0];
                u32_t apic_addr = ((u32_t*)(record->rest+2))[0];
                u32_t global_system_interrupt_base = ((u32_t*)(record->rest+2))[1];
                kprintf("[Kernel] I/O APIC ID=%x2 ADDR=%x8 INT_BASE=%x8\n", apic_id, apic_addr, global_system_interrupt_base);

                // This should probably be a recursive lock or something.
                pager.unlock();
                add_ioapic(apic_id, apic_addr, global_system_interrupt_base);
                pager.lock();
                break;
            } case 0x02: { // Interrupt Source Override Entry
                u8_t bus = record->rest[0];
                u8_t interrupt = record->rest[1];
                u32_t gsi = *((u32_t*)(record->rest+2));
                u16_t flags = *((u16_t*)(record->rest+6));
                kprintf("[Kernel] Int Source Override BUS=%x2 INT=%x2 GLOBAL=%x8 FLAGS=%x4\n", bus, interrupt, gsi, flags);

                pager.unlock();
                add_ioapic_intentry(interrupt+0x20, gsi, flags & 8, flags & 2, 0);
                pager.lock();
                break;
            } case 0x03: { // NMI Source Override Entry
                u8_t nmi = record->rest[0];
                u16_t flags = *((u16_t*)(record->rest+2));
                u32_t gsi = *((u32_t*)record->rest+4);
                kprintf("[Kernel] NMI Source Override NMI=%x2 GLOBAL=%x8 FLAGS=%x4\n", nmi, gsi, flags);

                break;
            }
        }
    }

    pager.unmap((virtaddr_t)madt, page_size);
}

struct gdt_entry {
    u16_t limit_015;
    u16_t base_015;
    u8_t base_1623;
    u8_t access;
    u8_t limit_1619:4;
    u8_t flags:4;
    u8_t base_2431;

} PACKED;

struct gdtr {
    u16_t size;
    u64_t base;
} PACKED;

struct TSS {
    u32_t reserved;
    u64_t rsp0;
    u64_t rsp1;
    u64_t rsp2;
    u32_t reserved1;
    u32_t reserved2;
    u64_t isp1;
    u64_t isp2;
    u64_t isp3;
    u64_t isp4;
    u64_t isp5;
    u64_t isp6;
    u64_t isp7;
    u32_t reserved3;
    u32_t reserved4;
    u16_t reserved5;
    u16_t iopb_offset;
} PACKED;

NO_EXPORT gdt_entry* gdt;
NO_EXPORT gdtr gdt_ptr;

NO_EXPORT TSS* tsses;

TEXT_FREE_AFTER_INIT void init_gdt() {
    gdt = new gdt_entry[5+core_count()*2];
    memset(&gdt[0], 0, sizeof(gdt_entry));

    // Kernel Code
    gdt[1].limit_015 = 0;
    gdt[1].base_015 = 0;
    gdt[1].base_1623 = 0;
    gdt[1].access = 0b10011010;
    gdt[1].limit_1619 = 0;
    gdt[1].flags = 0b1010;
    gdt[1].base_2431 = 0;

    // Kernel Data
    gdt[2].limit_015 = 0;
    gdt[2].base_015 = 0;
    gdt[2].base_1623 = 0;
    gdt[2].access = 0b10010010;
    gdt[2].limit_1619 = 0;
    gdt[2].flags = 0b1010;
    gdt[2].base_2431 = 0;

    // User Code
    gdt[3].limit_015 = 0;
    gdt[3].base_015 = 0;
    gdt[3].base_1623 = 0;
    gdt[3].access = 0b11111010;
    gdt[3].limit_1619 = 0;
    gdt[3].flags = 0b1010;
    gdt[3].base_2431 = 0;

    // User Data
    gdt[4].limit_015 = 0;
    gdt[4].base_015 = 0;
    gdt[4].base_1623 = 0;
    gdt[4].access = 0b11110010;
    gdt[4].limit_1619 = 0;
    gdt[4].flags = 0b1010;
    gdt[4].base_2431 = 0;

    // Create a Task State Segment for each core
    tsses = new TSS[core_count()];
    for(int i = 0; i < core_count(); ++i) {
        gdt[5+(i*2)].base_015 = (u64_t)&tsses[i];
        gdt[5+(i*2)].base_1623 = (u64_t)&tsses[i] >> 16;
        gdt[5+(i*2)].base_2431 = (u64_t)&tsses[i] >> 24;
        *((u64_t*)&gdt[5+(i*2)+1]) = (u64_t)&tsses[i] >> 32;
        gdt[5+(i*2)].limit_015 = sizeof(TSS);
        gdt[5+(i*2)].limit_1619 = 0;
        gdt[5+(i*2)].flags = 0b1000;
        gdt[5+(i*2)].access = 0b11101001;
    }

    gdt_ptr.size = sizeof(gdt_entry)*(5+core_count()*2);
    gdt_ptr.base = (u64_t)gdt;

    asm volatile(
        "lgdt %0\n"
        "pushq $8\n"
        "lea init_gdt_dest(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "init_gdt_dest: mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%ss\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov $0x2B, %%ax\n"
        "ltr %%ax"
    : : "m"(gdt_ptr) : "rax");
}

extern "C" void set_kernel_stack(int core, u64_t rsp) {
    tsses[core].rsp0 = rsp;
}

NO_EXPORT void crude_delay_1msec() {
    outb(0x43, 0b00110000);
    outb(0x40, 0xA9);
    outb(0x40, 0x04);

    while(true) {
        u16_t value = inb(0x40) | (inb(0x40) << 8);
        if(value == 0) break;
    }
}

extern "C" u8_t _binary_ap_starter_start[];
extern "C" u8_t _binary_ap_starter_end[];

extern "C" u64_t idtr;

inline void init_lapic_timer() {
    write_lapic(0x320, 0x000000FE);
}

TEXT_FREE_AFTER_INIT void core_init() {
    asm volatile("lidt %0" : : "m"(idtr));
    int core_id = current_core();

    init_lapic_timer();

    asm volatile(
        "lgdt %0\n"
        "pushq $8\n"
        "lea core_init_dest(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "core_init_dest: mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%ss\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "ltr %%cx\n"
        "sti\n"
        "int $0xFE"
    : : "m"(gdt_ptr), "c"(0x2B+(core_id*16)) : "rax");
    while(true);
}

NO_EXPORT u32_t lapic_timer_ticks;
TEXT_FREE_AFTER_INIT void measure_lapic_timer() {
    write_lapic(0x320, 0x000000FF);
    write_lapic(0x3E0, 0b1011);
    write_lapic(0x380, 0xFFFFFFFF);
    crude_delay_1msec();
    lapic_timer_ticks = 0xFFFFFFFF - read_lapic(0x390);
    write_lapic(0x380, 0);
    kprintf("[Kernel] 1 ms took %d Local APIC timer ticks\n", lapic_timer_ticks);
}

extern void init_time();

extern "C" TEXT_FREE_AFTER_INIT void init_cpu() {
    parse_madt();

    init_gdt();
    init_interrupts();

    init_pic();

    // Measure Local APIC timer ticks for 1 millisecond
    measure_lapic_timer();
    init_lapic_timer();

    Scheduler::init(core_count());

    auto& pager = Pager::active();
    pager.lock();
    pager.map(0x1000, 0x1000, 3, { 1, 1, 0, 1, 0, 0 });
    memcpy((void*)0x1000, _binary_ap_starter_start, _binary_ap_starter_end-_binary_ap_starter_start);
    *((u64_t*)0x2008) = pager.cr3();
    *((u64_t*)0x2010) = (u64_t)&core_init;
    for(auto item : core_map) {
        *((u64_t*)0x2000) = 0;
        if(item.value != 0) {
            kprintf("[Kernel] Starting core %d...\n", item.value);
            send_ipi(0x00, 0x5, item.key);
            for(int i = 0; i < 10; ++i) crude_delay_1msec();

            for(int i = 0; i < 2; ++i) {
                send_ipi(0x01, 0x6, item.key);
                crude_delay_1msec();
                if(*((u32_t*)0x2000) == 1) break;
            }
            while(!Scheduler::scheduler(item.value).is_idle());
        }
    }
    pager.unlock();

    init_time();
}

extern "C" int core_count() {
    return core_map.size();
}

extern "C" int current_core() {
    auto value = core_map.at(read_lapic(0x20) >> 24);
    if(value) return *value;
    else {
        ASSERT_NOT_REACHED("Running on core outside of our known core map");
        return -1;
    }
}
