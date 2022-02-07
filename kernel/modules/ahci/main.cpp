#include <modules/module_header.h>
#include <defines.h>
#include <stdlib.h>
#include <dmesg.h>
#include "../pci/pci_header.h"
#include "structures.hpp"
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

#define COMMAND_TABLE_PAGES 2
constexpr size_t max_prdt_size = (COMMAND_TABLE_PAGES*4096-128)/16;

struct drive_information {
    bool atapi;
    bool support64;
    Port_Command_List* command_list;
    Port_Command_Table* tables[32];
    kernel::SpinLock lock;
    HBA_MEM* hba;
    int port_id;
    u32_t ref_count;

    drive_information() {
        memset(tables, 0, sizeof(tables));
        ref_count = 0;
    }
};

struct drive_file {
    drive_information* drive;
    u64_t partition_start;
    u64_t partition_end;
    kernel::VNode* node;
};

size_t drive_count;
std::Vector<drive_file> drives;

size_t ahci_ata_read(drive_information* drive, u64_t lba, u16_t sector_count, void* buffer);

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
    drives.push_back({ drive_info, 0, 0 });

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

int get_cmd_slot(drive_information* drive) {
    u32_t slots = drive->hba->ports[drive->port_id].command_issue | drive->hba->ports[drive->port_id].sata_active;
    for(int i = 0; i < 32; ++i) {
        if(!((slots >> i) & 1)) {
            if(drive->tables[i] == 0) {
                physaddr_t addr = palloc(COMMAND_TABLE_PAGES);
                Port_Command_Header* header = &drive->command_list->command_headers[i];
                header->command_table_base = addr;
                if(drive->support64) header->command_table_base_upper = addr >> 32;

                kernel::Pager& pager = kernel::Pager::kernel();
                pager.lock();
                drive->tables[i] = (Port_Command_Table*)pager.kmap(addr, COMMAND_TABLE_PAGES, { .present = 1, .writable = 1, .user_accesible = 0, .executable = 0, .global = 1, .cache_disable = 1 });
                memset(drive->tables[i], 0, COMMAND_TABLE_PAGES*4096);
                pager.unlock();
            }
            return i;
        }
    }
    return -1;
}

size_t ahci_ata_read(drive_information* drive, u64_t lba, u16_t sector_count, void* buffer) {
    drive->lock.lock();
    int cmd_slot_index;
    while((cmd_slot_index = get_cmd_slot(drive)) == -1);

    Port_Command_Header* cmd_header = &drive->command_list->command_headers[cmd_slot_index];
    Port_Command_Table* cmd_table = drive->tables[cmd_slot_index];
    
    cmd_header->flags = sizeof(FIS_Host2Dev)/sizeof(u32_t);
    FIS_Host2Dev* fis = (FIS_Host2Dev*)cmd_table->command_fis;
    fis->type = 0x27;
    fis->reserved = 0;

    fis->flags = 0x80;
    fis->command = 0x25; // Read DMA Extended
    fis->device = (1 << 6);

    fis->lba1 = lba;
    fis->lba2 = lba >> 8;
    fis->lba3 = lba >> 16;
    fis->lba4 = lba >> 24;
    fis->lba5 = lba >> 32;
    fis->lba6 = lba >> 40;

    kernel::Pager& pager = kernel::Pager::active();
    pager.lock();

    // Fill the PRDT entries
    virtaddr_t current_addr = (virtaddr_t)buffer;
    size_t left_to_read = sector_count * 512;
    size_t prdt_entry = 0;
    while(left_to_read > 0 && prdt_entry < max_prdt_size) {
        physaddr_t start_phys = pager.getPhysicalAddress(current_addr);
        size_t entry_byte_count = 0;

        while(pager.getPhysicalAddress(current_addr) == start_phys+entry_byte_count && entry_byte_count < 4096*1024 && left_to_read > 0) {
            size_t offset = 4096 - (current_addr & 0xFFF);
            if(offset > left_to_read) offset = left_to_read;

            entry_byte_count += offset;
            current_addr += offset;
            left_to_read -= offset;
        }

        cmd_table->prdt[prdt_entry].data_base = start_phys;
        if(drive->support64) cmd_table->prdt[prdt_entry].data_base_upper = start_phys >> 32;
        cmd_table->prdt[prdt_entry].i_count = entry_byte_count - 1;
        ++prdt_entry;
    }

    pager.unlock();

    cmd_header->prdt_length = prdt_entry;
    size_t bytes_read = sector_count * 512 - left_to_read;
    size_t sectors_read = (bytes_read >> 9); //+ ((bytes_read & 0x1FF) == 0 ? 0 : 1);

    fis->count = sectors_read;

    HBA_Port* port = &drive->hba->ports[drive->port_id];
    port->command_issue |= (1 << cmd_slot_index);
    drive->lock.unlock();

    /// TODO: [02.02.2022] Use interrupts here.
    while((port->command_issue | port->sata_active) & (1 << cmd_slot_index));

    return sectors_read;
}
