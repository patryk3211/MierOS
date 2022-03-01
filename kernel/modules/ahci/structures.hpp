#pragma once

#include "ahci_structures.hpp"
#include <locking/spinlock.hpp>
#include <fs/vnode.hpp>

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
