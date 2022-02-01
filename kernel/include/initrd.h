#ifndef _MIEROS_KERNEL_INITRD_H
#define _MIEROS_KERNEL_INITRD_H

#if defined(__cplusplus)
extern "C" {
#endif

extern void set_initrd(void* ptr);
extern void* get_file(const char* name);
extern void** get_files(const char* name_wildcard);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_INITRD_H
