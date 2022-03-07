#pragma once

#include "structures.hpp"

extern size_t ahci_ata_read(drive_information* drive, u64_t lba, u16_t sector_count, void* buffer);
