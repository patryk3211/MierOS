#ifndef _MIEROS_KERNEL_CPU_H
#define _MIEROS_KERNEL_CPU_H

#if defined(__cplusplus)
extern "C" {
#endif

extern void early_init_cpu();
extern void init_cpu();

extern int core_count();
extern int current_core();

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_CPU_H
