#include <arch/time.h>
#include <arch/x86_64/acpi.h>
#include <arch/x86_64/hpet.h>
#include <memory/virtual.hpp>
#include <arch/interrupts.h>

#include <arch/x86_64/ports.h>

using namespace kernel;

void time_interrupt() {
    dmesg("Timer\n");
}

void init_time() {
    kprintf("PIT\n");
    /*outb(0x43, 0b00110000);
    outb(0x40, 0xA9);
    outb(0x40, 0x04);*/

    register_handler(0x20, &time_interrupt);

    auto& pager = Pager::active();
    pager.lock();

    physaddr_t hpet_table_addr = get_table("HPET");
    if(hpet_table_addr != 0) {
        auto* hpet_table = (ACPI_HPET_Table*)pager.kmap(hpet_table_addr, 2, { 1, 0, 0, 0, 0, 0 });

        size_t byte_size = (hpet_table_addr & 0xFFF) + hpet_table->header.length;
        size_t page_size = (byte_size >> 12) + ((byte_size & 0xFFF) == 0 ? 0 : 1);

        pager.unmap((virtaddr_t)hpet_table, 2);
        hpet_table = (ACPI_HPET_Table*)pager.kmap(hpet_table_addr, page_size, { 1, 0, 0, 0, 0, 0 });

        volatile HPET* hpet = (volatile HPET*)pager.kmap(hpet_table->address.address, 1, { 1, 1, 0, 0, 1, 0 });
        kprintf("HPET %x16 %x16 %x1 %x16\n", hpet->capabilities_and_id, hpet->configuration, hpet_table->comparatorCount, hpet->counter_value);
        kprintf("Timer 0 %x16 %x16\n", hpet->timers[0].comparator_value, hpet->timers[0].config_and_capabilities);
        kprintf("Timer 1 %x16 %x16\n", hpet->timers[1].comparator_value, hpet->timers[1].config_and_capabilities);
        kprintf("Timer 2 %x16 %x16\n", hpet->timers[2].comparator_value, hpet->timers[2].config_and_capabilities);

        u64_t period = hpet->capabilities_and_id >> 32;
        u64_t frequency = 1000000000000000 / period;

        hpet->timers[0].config_and_capabilities = 0b10001001100;
        hpet->timers[0].comparator_value = frequency;
        hpet->timers[0].comparator_value = frequency;

        hpet->counter_value = 0;
        hpet->configuration |= 1;

        kprintf("After Config\nHPET %x16 %x16 %x1 %x16\n", hpet->capabilities_and_id, hpet->configuration, hpet_table->comparatorCount, hpet->counter_value);
        kprintf("Timer 0 %x16 %x16\n", hpet->timers[0].comparator_value, hpet->timers[0].config_and_capabilities);
        kprintf("Timer 1 %x16 %x16\n", hpet->timers[1].comparator_value, hpet->timers[1].config_and_capabilities);
        kprintf("Timer 2 %x16 %x16\n", hpet->timers[2].comparator_value, hpet->timers[2].config_and_capabilities);
    }

    pager.unlock();
}

extern "C" void set_time(time_t time) {

}

extern "C" time_t get_time() {

}

extern "C" time_t get_uptime() {

}
