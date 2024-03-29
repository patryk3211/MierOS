#include "drive.hpp"

#include "ata.hpp"
#include "../../partition/parse.hpp"
#include <fs/devicefs.hpp>
#include <memory/physical.h>
#include <printf.h>

extern u16_t major;

void find_partitions(drive_information* drive, const std::String<>& disk_name, u16_t disk_minor) {
    if(!drive->atapi) {
        auto* parse_info = kernel::parse_partitions(
            512, 1, [drive](u64_t lba, void* buffer) { return ahci_ata_read(drive, lba, 1, buffer); }, [drive](u64_t lba, void* buffer) { return (size_t)0; });

        if(parse_info->type == GPT_PPI_TYPE) {
            auto* gpt = (kernel::GPTParttionParseInformation*)parse_info;

            {
                std::String<> disk_by_id = "block/by-id/";
                disk_by_id += std::uuid_to_string(gpt->disk_id);
                auto res = kernel::DeviceFilesystem::instance()->resolve_path(disk_by_id.c_str(), nullptr);
                if(res) {
                    char buffer[128];
                    sprintf(buffer, "../../%s", drives[disk_minor].node->name().c_str());
                    kernel::DeviceFilesystem::instance()->symlink(res->key, res->value, buffer);
                }
            }

            for(auto& part : *gpt) {
                u16_t minor = minor_num++;
                drives.insert({ minor, { drive, part.start_lba, part.end_lba, nullptr } });

                auto file = kernel::DeviceFilesystem::instance()->add_dev((disk_name + "p" + std::num_to_string(minor)).c_str(), major, minor, kernel::VNode::BLOCK_DEVICE);
                if(file) {
                    drives[minor].node = *file;

                    std::String<> part_by_id = "block/by-id/";
                    part_by_id += std::uuid_to_string(part.part_id);
                    auto res = kernel::DeviceFilesystem::instance()->resolve_path(part_by_id.c_str(), nullptr);
                    if(res) {
                        char buffer[128];
                        sprintf(buffer, "../../%s", drives[disk_minor].node->name().c_str());
                        kernel::DeviceFilesystem::instance()->symlink(res->key, res->value, buffer);
                    }
                }
            }
        }
    }
}

#define SATA_SIG_ATA 0x00000101
#define SATA_SIG_ATAPI 0xEB140101
#define SATA_SIG_SEMB 0xC33C0101
#define SATA_SIG_PM 0x96690101

void init_drive(HBA_MEM* hba, int port_id, kernel::Pager& pager, bool support64) {
    HBA_Port* port = &hba->ports[port_id];

    if((port->sata_status & 0xF) != 3) {
        dmesg("(AHCI) No device detected at port %d", port_id);
        return;
    }

    if(((port->sata_status >> 8) & 0xF) != 1) {
        dmesg("(AHCI) Device at port %d is not in an active state", port_id);
        return;
    }

    // Find out what kind of device is connected
    drive_information* drive_info = new drive_information();
    switch(port->signature) {
        case SATA_SIG_ATA:
            dmesg("(AHCI) Found ATA device at port %d", port_id);
            drive_info->atapi = false;
            break;
        case SATA_SIG_ATAPI:
            dmesg("(AHCI) Found ATAPI device at port %d", port_id);
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
            dmesg("(AHCI) Unknown device found at port %d", port_id);
            return;
    }

    drive_info->hba = hba;
    drive_info->port_id = port_id;
    drive_info->support64 = support64;

    // Stop command processor and FIS receiver
    port->command_status &= ~((1 << 4) | (1 << 0));
    while((port->command_status & (1 << 15)) || (port->command_status & (1 << 14)))
        ;

    // Allocate required structures
    physaddr_t command_list_p = palloc(1);
    drive_info->command_list = (Port_Command_List*)pager.kmap(command_list_p, 1, kernel::PageFlags(true, true, false, false, true, true));

    memset(drive_info->command_list, 0, sizeof(Port_Command_List));

    port->command_list_base = command_list_p;
    if(support64) port->command_list_base_upper = command_list_p >> 32;

    port->fis_base = command_list_p + sizeof(Port_Command_Header) * 32;
    if(support64) port->fis_base_upper = command_list_p >> 32;

    // Start command processor and FIS receiver
    while((port->command_status & (1 << 15)))
        ;
    port->command_status |= (1 << 4) | (1 << 0);

    u16_t minor = minor_num++;
    drives.insert({ minor, { drive_info, 0, 0, nullptr } });

    std::String<> drive_name = "ahci";
    drive_name += std::num_to_string(drive_count);

    auto ret = kernel::DeviceFilesystem::instance()->add_dev(drive_name.c_str(), major, minor, kernel::VNode::BLOCK_DEVICE);
    if(ret) {
        drives[minor].node = *ret;

        // We have to unlock the pager here, because it is later locked in the read function and would cause a deadlock.
        pager.unlock();
        find_partitions(drive_info, drive_name, minor);
        pager.lock();

        ++drive_count;
    }
}
