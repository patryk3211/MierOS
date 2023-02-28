#ifndef _ASM_IOCTLS_H
#define _ASM_IOCTLS_H

#define TCGETS      0x5401
#define TCSETS      0x5402
#define TCSETSW     0x5403
#define TCSETSF     0x5404

#define TIOCSCTTY   0x540e

#ifdef __KERNEL__
// These are provided by <termios.h> in mlibc
#define TIOCGPGRP   0x540f
#define TIOCSPGRP   0x5410
#define TIOCGWINSZ  0x5413
#define TIOCSWINSZ  0x5414
#define TIOCGSID    0x5429
#endif

#endif

