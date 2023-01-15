#pragma once

#define MMAP_PROT_NONE  0
#define MMAP_PROT_READ  1
#define MMAP_PROT_WRITE 2
#define MMAP_PROT_EXEC  4

#define MMAP_FLAG_PRIVATE   0
#define MMAP_FLAG_SHARED    1
#define MMAP_FLAG_ANONYMOUS 2
#define MMAP_FLAG_FIXED     4
#define MMAP_FLAG_ZERO      8

#define MMAP_MIN_ADDR 0x100000
