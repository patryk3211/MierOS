#ifndef _MIEROS_KERNEL_MODULES_MODULE_HEADER_H
#define _MIEROS_KERNEL_MODULES_MODULE_HEADER_H

#include <types.h>
#include <defines.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct module_header {
    char magic[8]; // magic = "MODHDRV1"
    char mod_name[128];
} PACKED;

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_MODULES_MODULE_HEADER_H
