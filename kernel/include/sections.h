#ifndef _MIEROS_KERNEL_SECTIONS_H
#define _MIEROS_KERNEL_SECTIONS_H

#define FREE_AFTER_INIT __attribute__((section(".free_after_init")))
#define TEXT_FREE_AFTER_INIT __attribute__((section(".text_free_after_init")))

#endif // _MIEROS_KERNEL_SECTIONS_H
