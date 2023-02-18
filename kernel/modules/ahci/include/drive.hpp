#pragma once

#include "structures.hpp"

void init_drive(HBA_MEM* hba, int port_id, kernel::Pager& pager, bool support64);
