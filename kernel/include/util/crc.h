#ifndef _MIEROS_KERNEL_CRC_H
#define _MIEROS_KERNEL_CRC_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern u32_t calc_crc32(const void* data, size_t length);

extern u32_t begin_calc_crc32();
extern u32_t continue_calc_crc32(u32_t remainder, const void* data, size_t length);
extern u32_t end_calc_crc32(u32_t remaider);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_CRC_H
