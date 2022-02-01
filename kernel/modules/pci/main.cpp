#include <modules/module_header.h>
#include <defines.h>
#include <list.hpp>
#include <dmesg.h>
#include <modules/module_manager.hpp>
#include "pci_header.h"

extern char header_mod_name[];
extern char init_on[];
MODULE_HEADER static module_header header {
    .header_version = 1, // Header Version 1
    .reserved1 = 0,
    .preferred_major = 0, // No preferred major number value
    .flags = 0,
    .reserved2 = 0,
    .dependencies_ptr = 0, // No dependencies
    .name_ptr = (u64_t)&header_mod_name, // Name
    .init_on_ptr = (u64_t)&init_on // Initialize on
};

MODULE_HEADER char header_mod_name[] = "pci";
MODULE_HEADER char init_on[] = "INIT\0";

std::List<PCI_Header> pci_headers;

extern void detect_pci();

const char lookup[] = "0123456789abcdef";
extern "C" int init() {
    dmesg("[PCI] Detecting PCI devices...\n");
    detect_pci();

    kprintf("[PCI] Found %d devices.\n", pci_headers.size());
    for(auto& header : pci_headers)
        kprintf("[PCI] Vendor=%x4 DeviceID=%x4 Class=%x2 Subclass=%x2 ProgIf=%x2 Rev=%x2\n", header.vendor_id, header.device_id, header.classcode, header.subclass, header.prog_if, header.revision);
    
    char init_signal[] = "PCI-V????-D????-C??S??P??R??";
    for(auto& header : pci_headers) {
        init_signal[5] = lookup[(header.vendor_id >> 12) & 0xF];
        init_signal[6] = lookup[(header.vendor_id >>  8) & 0xF];
        init_signal[7] = lookup[(header.vendor_id >>  4) & 0xF];
        init_signal[8] = lookup[(header.vendor_id >>  0) & 0xF];

        init_signal[11] = lookup[(header.device_id >> 12) & 0xF];
        init_signal[12] = lookup[(header.device_id >>  8) & 0xF];
        init_signal[13] = lookup[(header.device_id >>  4) & 0xF];
        init_signal[14] = lookup[(header.device_id >>  0) & 0xF];

        init_signal[17] = lookup[(header.classcode >> 4) & 0xF];
        init_signal[18] = lookup[(header.classcode >> 0) & 0xF];

        init_signal[20] = lookup[(header.subclass >> 4) & 0xF];
        init_signal[21] = lookup[(header.subclass >> 0) & 0xF];

        init_signal[23] = lookup[(header.prog_if >> 4) & 0xF];
        init_signal[24] = lookup[(header.prog_if >> 0) & 0xF];

        init_signal[26] = lookup[(header.revision >> 4) & 0xF];
        init_signal[27] = lookup[(header.revision >> 0) & 0xF];

        kprintf("[PCI] Initalizing modules for signal '%s'\n", init_signal);

        kernel::init_modules(init_signal, &header);
    }
    return 0;
}

extern "C" int destroy() {
    pci_headers.clear();
    return 0;
}
