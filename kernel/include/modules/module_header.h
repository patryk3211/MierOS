#ifndef _MIEROS_KERNEL_MODULES_MODULE_HEADER_H
#define _MIEROS_KERNEL_MODULES_MODULE_HEADER_H

#include <types.h>
#include <defines.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define MODULE_HEADER_MAGIC { 0x7F, 'M', 'O', 'D', 'H', 'D', 'R', '1' }

struct module_header {
    char magic[8]; // magic = "\0177MODHDR1"
    char mod_name[128];
    char** dependencies;
    char** init_on;
} PACKED;

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_MODULES_MODULE_HEADER_H
