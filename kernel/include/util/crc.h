#ifndef _MIEROS_KERNEL_CRC_H
#define _MIEROS_KERNEL_CRC_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern u32_t calc_crc32(const void* data, size_t length);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_CRC_H
