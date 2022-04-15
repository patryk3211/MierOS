#pragma once

#include "gpt.h"
#include <list.hpp>
#include <types.h>
#include <util/crc.h>

#define GPT_PPI_TYPE 0x01

namespace kernel {
    struct PartitionParseInformation {
        u8_t type;

        virtual ~PartitionParseInformation() { }
    };

    struct GPTParttionParseInformation : public PartitionParseInformation {
        struct Partition {
            uuid_t part_id;
            uuid_t part_type;
            u64_t start_lba;
            u64_t end_lba;
        };

        uuid_t disk_id;
        size_t partition_count;
        Partition* partitions;

        ~GPTParttionParseInformation() {
            delete partitions;
        }

        Partition* begin() { return partitions; }
        Partition* end() { return partitions + partition_count; }
    };

    template<typename ReadSector, typename WriteSector> PartitionParseInformation* parse_partitions(u32_t sector_size, u64_t disk_sector_count, ReadSector read_sector_func, WriteSector write_sector_func) {
        u8_t buffer[sector_size];
        read_sector_func(1, buffer);

        if(*((u64_t*)buffer) == 0x5452415020494645 /* "EFI PART" */) {
            // GPT Found
            GPT_Header* header = (GPT_Header*)buffer;
            { // Check for corruption
                u32_t checksum = header->header_checksum;
                header->header_checksum = 0;

                u32_t calc_checksum = calc_crc32(header, sizeof(GPT_Header));
                if(checksum != calc_checksum) {
                    // Header is corrupted!
                    /// TODO: [06.03.2022] Check the backup header.
                    return 0;
                }
                /// TODO: [06.03.2022] Check if the backup header is not corrupted and if it is, fix it.
            }

            GPTParttionParseInformation* return_info = new GPTParttionParseInformation();

            return_info->type = GPT_PPI_TYPE;
            return_info->disk_id = header->disk_uuid.to_uuid();

            // We parse the partition table while calculating the checksum, if the checksum validation fails we try to use the backup table and discard the parse result of this one.
            std::List<GPTParttionParseInformation::Partition> partitions;

            u32_t checksum = begin_calc_crc32();

            u8_t buffer2[512];
            size_t entries_per_sector = sector_size / header->partition_entry_size;
            for(size_t i = 0; i < (header->partition_count / entries_per_sector) + (header->partition_count % entries_per_sector == 0 ? 0 : 1); ++i) {
                read_sector_func(header->partition_table_lba + i, buffer2);
                checksum = continue_calc_crc32(checksum, buffer2, sector_size);
                for(size_t j = 0; j < entries_per_sector; ++j) {
                    GPT_PartitionEntry* partition = (GPT_PartitionEntry*)(buffer2 + j * header->partition_entry_size);
                    if(partition->partition_type.is_zero()) continue;

                    partitions.push_back({ partition->partition_uuid.to_uuid(), partition->partition_type.to_uuid(), partition->first_lba, partition->last_lba });
                }
            }

            checksum = end_calc_crc32(checksum);
            if(checksum != header->partition_table_checksum) {
                // Table is corrupted!
                /// TODO: [06.03.2022] Check the backup table.
                delete return_info;
                return 0;
            }
            /// TODO: [06.03.2022] Check if the backup table is not corrupted and if it is, fix it.

            // Convert partition list into an array.
            return_info->partition_count = partitions.size();
            return_info->partitions = new GPTParttionParseInformation::Partition[partitions.size()];

            size_t index = 0;
            for(auto& part : partitions)
                return_info->partitions[index++] = part;

            return return_info;
        } else {
            // Try MBR
            /// TODO: [06.03.2022] Implement
        }
        return 0;
    }
}
