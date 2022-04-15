#pragma once

#include <defines.h>
#include <types.h>

#define DIRENT_TYPE_UNKNOWN 0
#define DIRENT_TYPE_REGULAR 1
#define DIRENT_TYPE_DIRECTORY 2
#define DIRENT_TYPE_CHARDEV 3
#define DIRENT_TYPE_BLOCKDEV 4
#define DIRENT_TYPE_FIFO 5
#define DIRENT_TYPE_SOCKET 6
#define DIRENT_TYPE_SYMLINK 7

struct DirectoryEntry {
    u32_t inode;
    u16_t entry_size;
    u8_t name_length;
    u8_t type;
    char name[];
} PACKED;
