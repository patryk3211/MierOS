#ifndef _ASM_TERMIOS_H
#define _ASM_TERMIOS_H

typedef unsigned int cc_t;
typedef unsigned int speed_t;
typedef unsigned int tcflag_t;

#define NCCS       11

// Control characters
#define VEOF       0
#define VEOL       1
#define VERASE     2
#define VINTR      3
#define VKILL      4
#define VMIN       5
#define VQUIT      6
#define VSTART     7
#define VSTOP      8
#define VSUSP      9
#define VTIME      10

// Input flags
#define IGNBRK     1
#define BRKINT     2
#define IGNPAR     4
#define PARMRK     8
#define INPCK      16
#define ISTRIP     32
#define INLCR      64
#define IGNCR      128
#define ICRNL      256
#define IXON       512
#define IXANY      1024
#define IXOFF      2048

// Output flags
#define OPOST      1
#define ONLCR      2
#define OCRNL      4
#define ONOCR      8
#define ONLRET     16
#define OFILL      32
#define OFDEL      64

// Control flags
#define CS5        0
#define CS6        1
#define CS7        2
#define CS8        3
#define CSIZE      3
#define CSTOPB     4
#define CREAD      8
#define PARENB     16
#define PARODD     32
#define HUPCL      64
#define CLOCAL     128

// Local flags
#define ISIG       1
#define ICANON     2
#define ECHO       4
#define ECHOE      8
#define ECHOK      16
#define ECHONL     32
#define NOFLSH     64
#define TOSTOP     128
#define IEXTEN     256

struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_cc[NCCS];
	speed_t ibaud;
	speed_t obaud;
};

#endif

