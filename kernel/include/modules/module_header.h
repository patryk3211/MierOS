#ifndef _MIEROS_KERNEL_MODULES_MODULE_HEADER_H
#define _MIEROS_KERNEL_MODULES_MODULE_HEADER_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct module_header {
    u8_t header_version;
    u8_t reserved1;
    u16_t preferred_major;
    /**
     * @brief Flags that specify the module's properties
     * bit 0 = Fallback module - The module will only be loaded if no other module has responded on the init signal
     */
    u16_t flags;
    u16_t reserved2;
    u64_t dependencies_ptr;
    u64_t name_ptr;
    u64_t init_on_ptr;
};

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_MODULES_MODULE_HEADER_H
