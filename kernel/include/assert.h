#ifndef _MIEROS_KERNEL_ASSERT_H
#define _MIEROS_KERNEL_ASSERT_H

#include <dmesg.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define ASSERT(expr, msg) \
    if(!(expr)) kprintf("\033[1;31mASSERTION FAILED!\033[0m %s\n", msg)
#define ASSERT_F(expr, msg) \
    if(!(expr)) kprintf("\033[1;31mASSERTION FAILED! \033[1;34m%s\033[0m:\033[1;37m%d\033[0m %s\n", __FILE__, __LINE__, msg)
#define ASSERT_NOT_REACHED(msg) ASSERT_F(false, msg)

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_ASSERT_H
