#include "structures.hpp"
#include <memory/physical.h>
#include <memory/virtual.hpp>
#include "defines.hpp"

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
