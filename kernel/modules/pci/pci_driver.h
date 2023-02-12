#ifndef _MIEROS_MODULE_PCI_PCI_DRIVER_H
#define _MIEROS_MODULE_PCI_PCI_DRIVER_H

#include <types.h>
#include <defines.h>
#include "pci_header.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef void add_device_func_t(PCI_Header* header);

struct PCI_Driver {
    add_device_func_t* attach;
} PACKED;

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_MODULE_PCI_PCI_DRIVER_H
