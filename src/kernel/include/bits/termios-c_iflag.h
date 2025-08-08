#ifndef _TERMIOS_H
# error "Never include <bits/termios-ciflags.h> directly; use <termios.h> instead."
#endif

/* c_iflag bits */
#define IGNBRK	0000001  /* Ignore break condition.  */
#define BRKINT	0000002  /* Signal interrupt on break.  */
#define IGNPAR	0000004  /* Ignore characters with parity errors.  */
#define PARMRK	0000010  /* Mark parity and framing errors.  */
#define INPCK	0000020  /* Enable input parity check.  */
#define ISTRIP	0000040  /* Strip 8th bit off characters.  */
#define INLCR	0000100  /* Map NL to CR on input.  */
#define IGNCR	0000200  /* Ignore CR.  */
#define ICRNL	0000400  /* Map CR to NL on input.  */
#define IUCLC	0001000  /* Map uppercase characters to lowercase on input
			    (not in POSIX).  */
#define IXON	0002000  /* Enable start/stop output control.  */
#define IXANY	0004000  /* Enable any character to restart output.  */
#define IXOFF	0010000  /* Enable start/stop input control.  */
#define IMAXBEL	0020000  /* Ring bell when input queue is full
			    (not in POSIX).  */
#define IUTF8	0040000  /* Input is UTF8 (not in POSIX).  */
