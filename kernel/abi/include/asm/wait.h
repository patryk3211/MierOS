#ifndef _ASM_WAIT_H
#define _ASM_WAIT_H

#define WNOHANG         1
#define WUNTRACED       2
#define WSTOPPED        2
#define WEXITED         4
#define WCONTINUED      8
#define WNOWAIT         0x01000000

#define __WALL          0x40000000
#define __WCLONE        0x80000000

#define WCOREFLAG       0x10000

#define __EXITCODE_MASK 0xff
#define __REASON_MASK   0x300
#define __SIG_MASK      0xfc00

#define __REASON_EXIT   0x000
#define __REASON_SIGNAL 0x100
#define __REASON_STOP   0x200
#define __REASON_CONT   0x300

#define __VALID_FLAG    0x20000

#define W_ISVALID(x) ((x) & __VALID_FLAG)

#define WIFEXITED(x) (W_ISVALID(x) && ((x) & __REASON_MASK) == __REASON_EXIT)
#define WIFSIGNALLED(x) (W_ISVALID(x) && ((x) & __REASON_MASK) == __REASON_SIGNAL)
#define WIFSTOPPED(x) (W_ISVALID(x) && ((x) & __REASON_MASK) == __REASON_STOP)
#define WIFCONTINUED(x) (W_ISVALID(x) && ((x) & __REASON_MASK) == __REASON_CONT)

#define WEXITSTATUS(x) ((x) & __EXITCODE_MASK)
#define WTERMSIG(x) (((x) & __SIG_MASK) >> 10)
#define WSTOPSIG(x) WTERMSIG(x)
#define WCOREDUMP(x) ((x) & WCOREFLAG)

#define W_EXITCODE(ret, sig) (((sig) << 10) | (ret) | __VALID_FLAG)

#endif

