#ifndef _MIEROS_KERNEL_GPT_H
#define _MIEROS_KERNEL_GPT_H

#include <types.h>
#include <defines.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct GPT_UUID {
    u32_t part1;
    u16_t part2;
    u16_t part3;
    u8_t part4[2];
    u8_t part5[6];

#if defined(__cplusplus)
    uuid_t to_uuid() {
        u16_t temp_part4 = (part4[0] << 8) | part4[1];
        u64_t temp_part5 = ((u64_t)part5[0] << 40) | ((u64_t)part5[1] << 32) | ((u64_t)part5[2] << 24) | ((u64_t)part5[3] << 16) | ((u64_t)part5[4] << 8) | part5[5];

        return uuid_t(part1, part2, part3, temp_part4, temp_part5);
    }

    bool is_zero() {
        for(size_t i = 0; i < sizeof(GPT_UUID); ++i)
            if(((u8_t*)this)[i] != 0) return false;
        return true;
    }
#endif

} PACKED GPT_UUID;

struct GPT_Header {
    u8_t signature[8];
    u32_t revision;
    u32_t header_size;
    u32_t header_checksum;
    u32_t reserved;
    u64_t current_lba;
    u64_t backup_lba;
    u64_t first_usable_lba;
    u64_t last_usable_lba;
    GPT_UUID disk_uuid;
    u64_t partition_table_lba;
    u32_t partition_count;
    u32_t partition_entry_size;
    u32_t partition_table_checksum;
} PACKED;

struct GPT_PartitionEntry {
    GPT_UUID partition_type;
    GPT_UUID partition_uuid;
    u64_t first_lba;
    u64_t last_lba;
    u64_t flags;
    char partition_name[72];
} PACKED;

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_GPT_H
