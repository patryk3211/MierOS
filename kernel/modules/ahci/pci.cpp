#include "../pci/pci_header.h"
#include "../pci/pci_driver.h"
#include "structures.hpp"
#include "drive.hpp"
#include <defines.h>
#include <memory/virtual.hpp>
#include <locking/locker.hpp>

void init_pci_dev(PCI_Header* header) {
    kernel::Pager& pager = kernel::Pager::kernel();
    kernel::Locker lock(pager);

    HBA_MEM* hba = (HBA_MEM*)pager.kmap(header->bar[5] & ~0xFFF, 1, kernel::PageFlags(true, true, false, false, true, true));
    if(!(hba->capabilities & (1 << 18))) hba->global_host_control |= (1 << 31); // Enable AHCI mode

    bool support64 = (hba->capabilities >> 31) & 1;

    { // Check if our mapping is correct
        int biggest_implemented = -1;
        for(int i = 0; i < 32; ++i) {
            // Find out the last implemented port
            if((hba->ports_implemented >> i) & 1) biggest_implemented = i;
        }
        if(biggest_implemented >= 30) {
            // Remap the HBA memory to include the last ports.
            pager.unmap((virtaddr_t)hba, 1);
            hba = (HBA_MEM*)pager.kmap(header->bar[5] & ~0xFFF, 2, kernel::PageFlags(true, true, false, false, true, true));
        }
    }

    // Detect drives
    for(int i = 0; i < 32; ++i) {
        if((hba->ports_implemented >> i) & 1) {
            init_drive(hba, i, pager, support64);
        }
    }
}

USED PCI_Driver pci_driver {
    .attach = &init_pci_dev
};
