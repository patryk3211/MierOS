#include <arch/interrupts.h>
#include <arch/time.h>
#include <arch/x86_64/acpi.h>
#include <arch/x86_64/hpet.h>
#include <arch/x86_64/ports.h>
#include <locking/locker.hpp>
#include <memory/virtual.hpp>

using namespace kernel;

time_t uptime = 0;

time_t time_ms = 0;
time_t current_time = 0;

void time_interrupt() {
    uptime += 1;

    ++time_ms;
    if(time_ms == 1000) {
        time_ms = 0;
        ++current_time;
    }
}

void init_time() {
    dmesg("(Kernel) Initializing time");

    register_handler(0x20, &time_interrupt);

    physaddr_t hpet_table_addr = get_table("HPET");
    if(hpet_table_addr != 0) {
        dmesg("(Kernel) Using HPET for constant rate interrupts");

        auto& pager = Pager::active();
        Locker lock(pager);

        auto* hpet_table = (ACPI_HPET_Table*)pager.kmap(hpet_table_addr, 2, { 1, 0, 0, 0, 0, 0 });

        size_t byte_size = (hpet_table_addr & 0xFFF) + hpet_table->header.length;
        size_t page_size = (byte_size >> 12) + ((byte_size & 0xFFF) == 0 ? 0 : 1);

        pager.unmap((virtaddr_t)hpet_table, 2);
        hpet_table = (ACPI_HPET_Table*)pager.kmap(hpet_table_addr, page_size, { 1, 0, 0, 0, 0, 0 });

        volatile HPET* hpet = (volatile HPET*)pager.kmap(hpet_table->address.address, 1, { 1, 1, 0, 0, 1, 0 });

        u64_t period = hpet->capabilities_and_id >> 32;
        u64_t frequency = 1000000000000000 / period;
        u64_t timerValue = frequency / 1000; // 1 ms

        kprintf("[%T] (Kernel) HPET Frequency %d Hz\n", frequency);

        hpet->timers[0].config_and_capabilities = 0b10001001100;
        hpet->timers[0].comparator_value = timerValue;
        hpet->timers[0].comparator_value = timerValue;

        hpet->counter_value = 0;
        hpet->configuration |= 1;
    } else {
        dmesg("(Kernel) Falling back to PIT for constant rate interrupts");
        /// TODO: [14.04.2022] Implement PIT interrupts
    }
}

extern "C" void set_time(time_t time) {
    current_time = time;
}

extern "C" time_t get_time() {
    return current_time;
}

extern "C" time_t get_uptime() {
    return uptime;
}
