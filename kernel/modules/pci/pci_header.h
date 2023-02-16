#ifndef _MIEROS_MODULE_PCI_PCI_HEADER_H
#define _MIEROS_MODULE_PCI_PCI_HEADER_H

#include <types.h>
#include <fs/filesystem.hpp>
#include <fs/vnode.hpp>

#if defined(__cplusplus)
extern "C" {
#endif

struct PCI_Header {
    u8_t bus;
    u8_t device;
    u8_t function;
    u8_t zero;

    u16_t device_id;
    u16_t vendor_id;

    u8_t classcode;
    u8_t subclass;
    u8_t prog_if;
    u8_t revision;

    u32_t bar[6];
};

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_MODULE_PCI_PCI_HEADER_H
