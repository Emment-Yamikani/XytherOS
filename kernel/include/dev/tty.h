#pragma once

#include <dev/dev.h>
#include <ds/ringbuf.h>
#include <core/debug.h>
#include <sync/mutex.h>
#include <sync/rwlock.h>
#include <termios.h>

#define NTTY    8

typedef struct termios termios_t;

typedef struct winsize winsize_t;
struct winsize {
    unsigned short  ws_row;      /* rows, in characters */
    unsigned short  ws_col;      /* columns, in characters */
    unsigned short  ws_xpixel;   /* horizontal size, pixels (unused) */
    unsigned short  ws_ypixel;   /* vertical size, pixels (unused) */
};

#define TTYOPENED   0x0001
#define TTYBUSY     0x0002

#define TTY_IOBUFSZ     KiB(32)
#define TTY_DEVBUFSZ    KiB(8)

typedef struct tty_t {
    device_t    *t_dev;
    unsigned    t_flags;
    termios_t   t_termios;
    winsize_t   t_winsize;
    ringbuf_t   t_devbuf;
    ringbuf_t   t_iobuf;
    rwlock_t    t_rwlock;
} tty_t;

extern tty_t *tty_console;

#define tty_assert(tp)  ({ assert(tp, "Invalid tty."); })

extern int tty_alloc(tty_t **ptp);
extern void tty_free(tty_t *tp);

extern tty_t *tty_current(void);