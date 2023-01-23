#include "ata.hpp"
#include "defines.hpp"
#include "structures.hpp"
#include <defines.h>
#include <dmesg.h>
#include <fs/devicefs.hpp>
#include <locking/locker.hpp>
#include <memory/physical.h>
#include <memory/virtual.hpp>
#include <modules/module_header.h>
#include <stdlib.h>
#include <tasking/thread.hpp>
#include <unordered_map.hpp>

MODULE_HEADER static char __header_dep_pci[] = "pci";
MODULE_HEADER static char* __header_dependencies[] = {
    __header_dep_pci,
    0
};

MODULE_HEADER static module_header header {
    .magic = MODULE_HEADER_MAGIC,
    .mod_name = "ahci",
    .dependencies = __header_dependencies
};

extern u16_t major;

extern "C" int init() {
    drive_count = 0;
    minor_num = 0;

    return 0;
}

extern "C" int destroy() {
    kernel::Pager& pager = kernel::Pager::kernel();
    kernel::Locker lock(pager);

    for(auto file : drives) {
        if(file.value.partition_start == 0) {
            // If the partition starts at address 0 then this is the main drive file.
            for(int i = 0; i < 32; ++i)
                if(file.value.drive->tables[i] != 0)
                    pager.free((virtaddr_t)file.value.drive->tables[i], COMMAND_TABLE_PAGES);
            pager.free((virtaddr_t)file.value.drive->command_list, 1);
            delete file.value.drive;
        }
    }

    return 0;
}
