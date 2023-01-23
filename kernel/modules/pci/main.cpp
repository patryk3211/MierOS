#include "pci_header.h"
#include "pci_driver.h"
#include <defines.h>
#include <dmesg.h>
#include <list.hpp>
#include <modules/module_header.h>
#include <event/event_manager.hpp>
#include <event/kernel_events.hpp>
#include <modules/module.hpp>

MODULE_HEADER static module_header header {
    .magic = MODULE_HEADER_MAGIC,
    .mod_name = "pci",
    .dependencies = 0
};

std::List<PCI_Header> pci_headers;

extern void detect_pci();

void mod_load_cb(kernel::Module* mod, void* arg) {
    PCI_Driver* driver = (PCI_Driver*)mod->get_symbol_ptr("pci_driver");
    driver->add((PCI_Header*)arg);
}

const char lookup[] = "0123456789abcdef";
extern "C" int init() {
    dmesg("(PCI) Detecting PCI devices...");
    detect_pci();

    kprintf("[%T] (PCI) Found %d devices.\n", pci_headers.size());
    for(auto& header : pci_headers)
        kprintf("[%T] (PCI) Vendor=%x4 DeviceID=%x4 Class=%x2 Subclass=%x2 ProgIf=%x2 Rev=%x2\n", header.vendor_id, header.device_id, header.classcode, header.subclass, header.prog_if, header.revision);

    char init_signal[] = "PCI-V???\?-D???\?-C??S??P??R??";
    for(auto& header : pci_headers) {
        init_signal[5] = lookup[(header.vendor_id >> 12) & 0xF];
        init_signal[6] = lookup[(header.vendor_id >> 8) & 0xF];
        init_signal[7] = lookup[(header.vendor_id >> 4) & 0xF];
        init_signal[8] = lookup[(header.vendor_id >> 0) & 0xF];

        init_signal[11] = lookup[(header.device_id >> 12) & 0xF];
        init_signal[12] = lookup[(header.device_id >> 8) & 0xF];
        init_signal[13] = lookup[(header.device_id >> 4) & 0xF];
        init_signal[14] = lookup[(header.device_id >> 0) & 0xF];

        init_signal[17] = lookup[(header.classcode >> 4) & 0xF];
        init_signal[18] = lookup[(header.classcode >> 0) & 0xF];

        init_signal[20] = lookup[(header.subclass >> 4) & 0xF];
        init_signal[21] = lookup[(header.subclass >> 0) & 0xF];

        init_signal[23] = lookup[(header.prog_if >> 4) & 0xF];
        init_signal[24] = lookup[(header.prog_if >> 0) & 0xF];

        init_signal[26] = lookup[(header.revision >> 4) & 0xF];
        init_signal[27] = lookup[(header.revision >> 0) & 0xF];

        kprintf("[%T] (PCI) Initalizing modules for signal '%s'\n", init_signal);

        kernel::Event* event = new kernel::Event(EVENT_LOAD_MODULE, init_signal, &mod_load_cb, &header, 0);
        kernel::EventManager::get().raise(event);
    }
    return 0;
}

extern "C" int destroy() {
    pci_headers.clear();
    return 0;
}
