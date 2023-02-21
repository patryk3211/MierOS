#pragma once

#include <defines.h>
#include <types.h>

struct HBA_Port {
    u32_t command_list_base;
    u32_t command_list_base_upper;
    u32_t fis_base;
    u32_t fis_base_upper;
    u32_t interrupt_status;
    u32_t interrupt_enable;
    u32_t command_status;
    u32_t reserved;
    u32_t task_file_data;
    u32_t signature;
    u32_t sata_status;
    u32_t sata_control;
    u32_t sata_error;
    u32_t sata_active;
    u32_t command_issue;
    u32_t sata_notification;
    u32_t fis_switch_control;
    u32_t device_sleep;
    u32_t reserved2[10];
    u32_t vendor_specific[4];
} PACKED;

struct HBA_MEM {
    u32_t capabilities;
    u32_t global_host_control;
    u32_t interrupt_status;
    u32_t ports_implemented;
    u32_t version;
    u32_t ccc_control;
    u32_t ccc_ports;
    u32_t em_location;
    u32_t em_control;
    u32_t extended_capabilities;
    u32_t bios_handoff;

    u8_t reserved[212];

    HBA_Port ports[];
} PACKED;

struct Port_Command_Header {
    u16_t flags;
    u16_t prdt_length;
    u32_t prdt_byte_count;
    u32_t command_table_base;
    u32_t command_table_base_upper;
    u32_t reserved[4];
} PACKED;

// Type 0x27
struct FIS_Host2Dev {
    u8_t type;
    u8_t flags;
    u8_t command;
    u8_t features;
    u8_t lba1;
    u8_t lba2;
    u8_t lba3;
    u8_t device;
    u8_t lba4;
    u8_t lba5;
    u8_t lba6;
    u8_t features_upper;
    u16_t count;
    u8_t icc;
    u8_t control;
    u32_t reserved;
} PACKED;

// Type 0x34
struct FIS_Dev2Host {
    u8_t type;
    u8_t flags;
    u8_t status;
    u8_t error;
    u8_t lba1;
    u8_t lba2;
    u8_t lba3;
    u8_t device;
    u8_t lba4;
    u8_t lba5;
    u8_t lba6;
    u8_t reserved;
    u16_t count;
    u16_t reserved2;
    u32_t reserved3;
} PACKED;

// Type 0xA1
struct FIS_SetDevBits {
    u8_t type;
    u8_t flags;
    u16_t bits;
    u32_t protocol_specific;
} PACKED;

// Type 0x41
struct FIS_DMA_Setup {
    u8_t type;
    u8_t flags;
    u16_t reserved;
    u32_t dma_buffer_ident;
    u32_t dma_buffer_ident_upper;
    u32_t reserved2;
    u32_t dma_buffer_offset;
    u32_t dma_buffer_count;
    u32_t reserved3;
} PACKED;

// Type 0x5F
struct FIS_PIO_Setup {
    u8_t type;
    u8_t flags;
    u8_t status;
    u8_t error;
    u8_t lba1;
    u8_t lba2;
    u8_t lba3;
    u8_t device;
    u8_t lba4;
    u8_t lba5;
    u8_t lba6;
    u8_t reserved;
    u16_t count;
    u8_t reserved2;
    u8_t e_status;
    u16_t transfer_count;
    u16_t reserved3;
} PACKED;

struct Received_FIS_Table {
    FIS_DMA_Setup dma_setup;
    u8_t reserved1[4];

    FIS_PIO_Setup pio_setup;
    u8_t reserved2[12];

    FIS_Dev2Host d2h;
    u8_t reserved3[4];

    FIS_SetDevBits sdb;
    u8_t unknown_fis[64];

    u8_t reserved4[96];
} PACKED;

struct PRDT_Entry {
    u32_t data_base;
    u32_t data_base_upper;
    u32_t reserved;
    u32_t i_count;
} PACKED;

struct Port_Command_Table {
    u8_t command_fis[64];

    u8_t atapi_cmd[16];

    u8_t reserved[48];

    PRDT_Entry prdt[];
} PACKED;

struct Port_Command_List {
    Port_Command_Header command_headers[32];
    Received_FIS_Table fis_table;
} PACKED;
