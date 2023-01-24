#ifndef _MIEROS_KERNEL_UUID_H
#define _MIEROS_KERNEL_UUID_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern uuid_t str_to_uuid(const char* str);

#if defined(__cplusplus)
}
#endif

#endif // _MIEROS_KERNEL_UUID_H
