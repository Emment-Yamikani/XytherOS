#pragma once

#include <core/types.h>

#ifndef _TERMIOS_H
#define _TERMIOS_H 1


/* Get the system-dependent definitions of `struct termios', `tcflag_t',
   `cc_t', `speed_t', and all the macros specifying the flag bits.  */
#include <bits/termios.h>


/* Terminal ioctls */
#define TCGETS      0x5401  /* Get terminal attributes */
#define TCSETS      0x5402  /* Set terminal attributes */
#define TIOCINQ     0x541B  /* Get # bytes in input buffer */
#define TIOCOUTQ    0x5411  /* Get # bytes in output buffer */

#define _POSIX_VDISABLE '\0'

#endif // termios.h