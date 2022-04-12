#include <arch/time.h>
#include <arch/x86_64/acpi.h>

void init_time() {
    physaddr_t hpet_table = get_table("HPET");
}

extern "C" void set_time(time_t time) {

}

extern "C" time_t get_time() {

}

extern "C" time_t get_uptime() {

}
