#include <modules/module_header.h>
#include <defines.h>
#include <stdlib.h>
#include <dmesg.h>
#include "../pci/pci_header.h"
#include "structures.hpp"
#include "defines.hpp"
#include <memory/virtual.hpp>
#include <locking/locker.hpp>
#include <memory/physical.h>
#include <vector.hpp>
#include <fs/devicefs.hpp>
#include <tasking/thread.hpp>

#define	SATA_SIG_ATA    0x00000101
#define	SATA_SIG_ATAPI  0xEB140101
#define	SATA_SIG_SEMB   0xC33C0101
#define	SATA_SIG_PM	    0x96690101

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

MODULE_HEADER char header_mod_name[] = "ahci";
MODULE_HEADER char init_on[] = "PCI-V???\?-D???\?-C01S06P01R??\0";

size_t drive_count;
std::Vector<drive_file> drives;

extern size_t ahci_ata_read(drive_information* drive, u64_t lba, u16_t sector_count, void* buffer);

void find_partitions(drive_information* drive) {

}

void init_drive(HBA_MEM* hba, int port_id, kernel::Pager& pager, bool support64) {
    HBA_Port* port = &hba->ports[port_id];

    if((port->sata_status & 0xF) != 3) {
        kprintf("[AHCI] No device detected at port %d\n", port_id);
        return;
    }

    if(((port->sata_status >> 8) & 0xF) != 1) {
        kprintf("[AHCI] Device at port %d is not in an active state\n", port_id);
        return;
    }

    // Find out what kind of device is connected
    drive_information* drive_info = new drive_information();
    switch(port->signature) {
        case SATA_SIG_ATA:
            kprintf("[AHCI] Found ATA device at port %d\n", port_id);
            drive_info->atapi = false;
            break;
        case SATA_SIG_ATAPI:
            kprintf("[AHCI] Found ATAPI device at port %d\n", port_id);
            drive_info->atapi = true;
            break;
        case SATA_SIG_SEMB:
            panic("[AHCI] Enclosure managment bridge! Don't know what to do");
            break;
        case SATA_SIG_PM:
            panic("[AHCI] Port multiplier! Don't know what to do");
            break;
        default:
            delete drive_info;
            kprintf("[AHCI] Unknown device found at port %d\n", port_id);
            return;
    }

    drive_info->hba = hba;
    drive_info->port_id = port_id;
    drive_info->support64 = support64;

    // Stop command processor and FIS receiver
    port->command_status &= ~((1 << 4) | (1 << 0));
    while((port->command_status & (1 << 15)) || (port->command_status & (1 << 14)));

    // Allocate required structures
    physaddr_t command_list_p = palloc(1);
    drive_info->command_list = (Port_Command_List*)pager.kmap(command_list_p, 1, { .present = 1, .writable = 1, .user_accesible = 0, .executable = 0, .global = 1, .cache_disable = 1 });

    memset(drive_info->command_list, 0, sizeof(Port_Command_List));

    port->command_list_base = command_list_p;
    if(support64) port->command_list_base_upper = command_list_p >> 32;

    port->fis_base = command_list_p + sizeof(Port_Command_Header) * 32;
    if(support64) port->fis_base_upper = command_list_p >> 32;

    // Start command processor and FIS receiver
    while((port->command_status & (1 << 15)));
    port->command_status |= (1 << 4) | (1 << 0);

    size_t minor = drives.size();
    drives.push_back({ drive_info, 0, 0, 0 });

    char drive_name[] = "ahci?";
    drive_name[4] = '0' + drive_count;

    auto ret = kernel::DeviceFilesystem::instance()->add_dev(drive_name, kernel::Thread::current()->current_module->major(), minor);
    if(ret) {
        drives[minor].node = *ret;
        find_partitions(drive_info);

        ++drive_count;
    }
}

/// TODO: [02.02.2022] Since there can be multiple AHCI controllers we have to put the HBA's and all other objects in a list or something.
extern "C" int init(PCI_Header* header) {
    // We will maintain a lock for the duration of AHCI module initialization
    kernel::Pager& pager = kernel::Pager::kernel();
    kernel::Locker lock(pager);

    drive_count = 0;
    
    HBA_MEM* hba = (HBA_MEM*)pager.kmap(header->bar[5] & ~0xFFF, 1, { .present = 1, .writable = 1, .user_accesible = 0, .executable = 0, .global = 1, .cache_disable = 1 });
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
            hba = (HBA_MEM*)pager.kmap(header->bar[5] & ~0xFFF, 2, { .present = 1, .writable = 1, .user_accesible = 0, .executable = 0, .global = 1, .cache_disable = 1 });
        }
    }

    // Detect drives
    for(int i = 0; i < 32; ++i) {
        if((hba->ports_implemented >> i) & 1) {
            init_drive(hba, i, pager, support64);
        }
    }

    return 0;
}

extern "C" int destroy() {
    kernel::Pager& pager = kernel::Pager::kernel();
    kernel::Locker lock(pager);

    for(auto& file : drives) {
        if(file.partition_start == 0) {
            // If the partition starts at address 0 then this is the main drive file.
            for(int i = 0; i < 32; ++i)
                if(file.drive->tables[i] != 0)
                    pager.free((virtaddr_t)file.drive->tables[i], COMMAND_TABLE_PAGES);
            pager.free((virtaddr_t)file.drive->command_list, 1);
            delete file.drive;
        }
    }
    return 0;
}


