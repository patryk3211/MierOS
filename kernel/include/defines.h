#ifndef _MIEROS_KERNEL_SECTIONS_H
#define _MIEROS_KERNEL_SECTIONS_H

#define SECTION(s) __attribute__((section(s)))

#define FREE_AFTER_INIT __attribute__((section(".free_after_init")))
#define TEXT_FREE_AFTER_INIT __attribute__((section(".text_free_after_init")))

#define PACKED __attribute__((packed))

#endif // _MIEROS_KERNEL_SECTIONS_H
