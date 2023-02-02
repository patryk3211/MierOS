#pragma once

#define MODULE_HEADER_MAGIC { 0x7F, 'M', 'O', 'D', 'H', 'D', 'R', '1' }

struct ModuleHeader {
    char magic[8];
    char mod_name[128];
    char** dependencies;
    char** init_on;
}__attribute__((packed));

