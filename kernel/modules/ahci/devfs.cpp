#include "ata.hpp"
#include <defines.h>
#include <fs/devicefs.hpp>
#include <unordered_map.hpp>

using namespace kernel;

ValueOrError<u32_t> dev_block_read(u16_t minor, u64_t lba, u32_t sector_count, void* buffer);

DevFsFunctionTable dev_func_tab {
    .block_read = dev_block_read
};

extern std::UnorderedMap<u16_t, drive_file> drives;

ValueOrError<u32_t> dev_block_read(u16_t minor, u64_t lba, u32_t sector_count, void* buffer) {
    auto dev = drives.at(minor);
    if(!dev) return ERR_DEVICE_DOES_NOT_EXIST;

    // Boundary check
    u64_t part_len = dev->partition_end - dev->partition_start + 1;
    if(lba >= part_len) return 0;
    if(lba + sector_count >= part_len) sector_count = part_len - lba;

    u8_t* bbuffer = (u8_t*)buffer;
    if(!dev->drive->atapi) {
        u32_t read_count = 0;
        while(read_count < sector_count) {
            u16_t cur_read_count = sector_count - read_count;

            size_t act_read_count = ahci_ata_read(dev->drive, lba + dev->partition_start, cur_read_count, bbuffer);
            if(act_read_count != cur_read_count) {
                read_count += act_read_count;
                break;
            }

            bbuffer += cur_read_count * 512; /// TODO: [07.03.2022] Sector size should not be hard coded.
            read_count += cur_read_count;
            lba += cur_read_count;
        }
        return read_count;
    } else {
        /// TODO: [07.03.2022] Implement ATAPI read.
        return 0;
    }
}