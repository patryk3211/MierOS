#ifndef _MIEROS_KERNEL_TRACE_H
#define _MIEROS_KERNEL_TRACE_H

#include <types.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct file_line_pair {
    char* name;
    unsigned int line;
};

void set_line_map(void* map);
struct file_line_pair addr_to_line(u64_t address);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // _MIEROS_KERNEL_TRACE_H
