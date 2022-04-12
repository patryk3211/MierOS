#ifndef _MIEROS_KERNEL_ARCH_TIME_H
#define _MIEROS_KERNEL_ARCH_TIME_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern void set_time(time_t time);
extern time_t get_time();

extern time_t get_uptime();

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ARCH_TIME_H
