#include <arch/time.h>
#include <arch/x86_64/acpi.h>
#include <arch/x86_64/hpet.h>
#include <memory/virtual.hpp>

using namespace kernel;

void init_time() {
    auto& pager = Pager::active();
    pager.lock();

    physaddr_t hpet_table = get_table("HPET");
    if(hpet_table != 0) {
        auto* hpet = (ACPI_HPET_Table*)pager.kmap(hpet_table, 2, { 1, 0, 0, 0, 0, 0 });

        size_t byte_size = (hpet_table & 0xFFF) + hpet->header.length;
        size_t page_size = (byte_size >> 12) + ((byte_size & 0xFFF) == 0 ? 0 : 1);

        pager.unmap((virtaddr_t)hpet, 2);
        hpet = (ACPI_HPET_Table*)pager.kmap(hpet_table, page_size, { 1, 0, 0, 0, 0, 0 });
    }

    pager.unlock();
}

extern "C" void set_time(time_t time) {

}

extern "C" time_t get_time() {

}

extern "C" time_t get_uptime() {

}
