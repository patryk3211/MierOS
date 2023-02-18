#include "pci_header.h"
#include "pci_device.hpp"
#include <arch/x86_64/ports.h>
#include <list.hpp>

#include <dmesg.h>

u32_t pci_read(u8_t bus, u8_t device, u8_t func, u8_t offset) {
    outl(0xCF8, (1 << 31) | ((u32_t)bus << 16) | (((u32_t)device & 0x1F) << 11) | (((u32_t)func & 0x7) << 8) | ((u32_t)offset & 0xFC));
    return inl(0xCFC) >> ((offset & 0x3) << 3);
}

extern std::List<PCI_Device> pci_devices;

void detect_pci() {
    for(int bus = 0; bus < 256; ++bus) {
        for(int device = 0; device < 32; ++device) {
            for(int func = 0; func < 8; ++func) {
                u16_t vendor_id = pci_read(bus, device, func, 0x00);
                if(vendor_id != 0xFFFF) {
                    PCI_Header header {
                        .bus = (u8_t)bus,
                        .device = (u8_t)device,
                        .function = (u8_t)func,
                        .zero = 0
                    };

                    header.vendor_id = vendor_id;
                    header.device_id = pci_read(bus, device, func, 0x02);

                    u32_t device_type = pci_read(bus, device, func, 0x08);
                    header.classcode = device_type >> 24;
                    header.subclass = device_type >> 16;
                    header.prog_if = device_type >> 8;
                    header.revision = device_type;

                    u8_t header_type = pci_read(bus, device, func, 0x0E);
                    if((header_type & 0x7F) == 0) {
                        header.bar[0] = pci_read(bus, device, func, 0x10);
                        header.bar[1] = pci_read(bus, device, func, 0x14);
                        header.bar[2] = pci_read(bus, device, func, 0x18);
                        header.bar[3] = pci_read(bus, device, func, 0x1C);
                        header.bar[4] = pci_read(bus, device, func, 0x20);
                        header.bar[5] = pci_read(bus, device, func, 0x24);

                        u32_t subsys = pci_read(bus, device, func, 0x2C);
                        header.subsys_device = subsys >> 16;
                        header.subsys_vendor = subsys;

                        pci_devices.push_back({ header });
                    }
                }
            }
        }
    }
}
