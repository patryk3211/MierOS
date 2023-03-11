#ifndef _ASM_STAT_H
#define _ASM_STAT_H

#if defined(__cplusplus)
extern "C" {
#endif

struct mieros_stat {
    unsigned int f_mode;
    unsigned short f_uid;
    unsigned short f_gid;

    unsigned int f_links;
    unsigned int f_dev;
    unsigned long f_ino;
    unsigned int f_rdev;

    unsigned long f_size;
    unsigned int f_blksize;
    unsigned long f_blocks;

    unsigned long f_atime;
    unsigned long f_mtime;
    unsigned long f_ctime;
};

#if defined(__cplusplus)
}
#endif

#endif

