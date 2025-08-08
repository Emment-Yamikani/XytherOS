#ifndef _TERMIOS_H
# error "Never include <bits/termios-c_cflag.h> directly; use <termios.h> instead."
#endif

/* c_cflag bits.  */
#define CSIZE	0000060
#define   CS5	0000000
#define   CS6	0000020
#define   CS7	0000040
#define   CS8	0000060
#define CSTOPB	0000100
#define CREAD	0000200
#define PARENB	0000400
#define PARODD	0001000
#define HUPCL	0002000
#define CLOCAL	0004000

#ifdef __USE_MISC
# define ADDRB 04000000000
#endif
