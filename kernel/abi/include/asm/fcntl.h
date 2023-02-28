#ifndef _ASM_FCNTL_H
#define _ASM_FCNTL_H

#define AT_FDCWD -1

#define O_ACCMODE 0x0007
#define O_EXEC 1
#define O_RDONLY 2
#define O_RDWR 3
#define O_SEARCH 4
#define O_WRONLY 5

#define O_APPEND    0x000008
#define O_CREAT     0x000010
#define O_DIRECTORY 0x000020
#define O_EXCL      0x000040
#define O_NOCTTY    0x000080
#define O_NOFOLLOW  0x000100
#define O_TRUNC     0x000200
#define O_NONBLOCK  0x000400
#define O_DSYNC     0x000800
#define O_RSYNC     0x001000
#define O_SYNC      0x002000
#define O_CLOEXEC   0x004000
#define O_PATH      0x008000
#define O_LARGEFILE 0x010000
#define O_NOATIME   0x020000
#define O_ASYNC     0x040000
#define O_TMPFILE   0x080000
#define O_DIRECT    0x100000

#define DUP_SOFT    0x000001

#define F_DUPFD 1
#define F_DUPFD_CLOEXEC 2
#define F_GETFD 3
#define F_SETFD 4
#define F_GETFL 5
#define F_SETFL 6
#define F_GETLK 7
#define F_SETLK 8
#define F_SETLKW 9
#define F_GETOWN 10
#define F_SETOWN 11

#define FD_CLOEXEC 1

#define FDF_OP_MASK 0x70000000
#define FDF_GETS    0x10000000
#define FDF_GETD    0x20000000
#define FDF_SETS    0x30000000
#define FDF_SETD    0x40000000

#define F_SEAL_SHRINK 0x0002
#define F_SEAL_GROW   0x0004
#define F_SEAL_WRITE  0x0008
#define F_SEAL_SEAL   0x0010
#define F_ADD_SEALS   1033
#define F_GET_SEALS   1034

#define AT_EMPTY_PATH 1
#define AT_SYMLINK_FOLLOW 2
#define AT_SYMLINK_NOFOLLOW 4
#define AT_REMOVEDIR 8
#define AT_EACCESS 512

#define F_RDLCK 1
#define F_UNLCK 2
#define F_WRLCK 3

#endif

