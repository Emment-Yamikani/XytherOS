#ifndef _TERMIOS_H
# error "Never include <bits/termios.h> directly; use <termios.h> instead."
#endif

typedef unsigned char	cc_t;
typedef unsigned int	speed_t;
typedef unsigned int	tcflag_t;

#include <bits/termios-struct.h>
#include <bits/termios-c_cc.h>
#include <bits/termios-c_iflag.h>
#include <bits/termios-c_oflag.h>

/* c_cflag bit meaning */
#define  B0	      0000000   /* hang up */
#define  B50      0000001
#define  B75      0000002
#define  B110     0000003
#define  B134     0000004
#define  B150     0000005
#define  B200     0000006
#define  B300     0000007
#define  B600     0000010
#define  B1200    0000011
#define  B1800    0000012
#define  B2400    0000013
#define  B4800    0000014
#define  B9600    0000015
#define  B19200	  0000016
#define  B38400	  0000017
#ifdef __USE_MISC
# define EXTA     B19200
# define EXTB     B38400
#endif
#include <bits/termios-baud.h>

#include <bits/termios-c_cflag.h>
#include <bits/termios-c_lflag.h>

#ifdef __USE_MISC
/* ioctl (fd, TIOCSERGETLSR, &result) where result may be as below */
# define TIOCSER_TEMT    0x01   /* Transmitter physically empty */
#endif

/* tcflow() and TCXONC use these */
#define	TCOOFF		0
#define	TCOON		1
#define	TCIOFF		2
#define	TCION		3

/* tcflush() and TCFLSH use these */
#define	TCIFLUSH	0
#define	TCOFLUSH	1
#define	TCIOFLUSH	2

#include <bits/termios-tcflow.h>

#include <bits/termios-misc.h>
