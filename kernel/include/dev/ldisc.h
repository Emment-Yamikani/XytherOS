#pragma once

#include <core/types.h>

struct tty;
struct termios;

typedef struct ldisc {
    char    *name;
    int     magic;

    int     (*open)(struct tty *tp);
    void    (*close)(struct tty *tp);
    void    (*flush_buffer)(struct tty *tp);
    ssize_t (*chars_in_buffer)(struct tty *tp);
    ssize_t (*read)(struct tty *tp, char *buf, size_t nr);
    ssize_t (*write)(struct tty *tp, const char *buf, size_t nr);
    int     (*ioctl)(struct tty *tp, uint cmd, void *arg);
    void    (*set_termios)(struct tty *tp, struct termios *old);
    int     (*receive_buf)(struct tty *tp, const char *cp, char *fp, int count);

} ldisc_t;

enum ldisciplines {
    x_TTY = 0,
    // TODO: add others
};

extern ldisc_t ldisc_X_TTY;

extern int ldisc_init();
extern int register_ldisc(int id, ldisc_t *ldisc);
