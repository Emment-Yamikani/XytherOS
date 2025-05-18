#pragma once

#include <dev/dev.h>
#include <dev/ldisc.h>
#include <ds/ringbuf.h>
#include <core/debug.h>
#include <sync/event.h>
#include <sync/spinlock.h>
#include <termios.h>

#define NTTY    256

typedef struct termios termios_t;

typedef struct winsize winsize_t;
struct winsize {
    unsigned short  ws_row;      /* rows, in characters */
    unsigned short  ws_col;      /* columns, in characters */
    unsigned short  ws_xpixel;   /* horizontal size, pixels (unused) */
    unsigned short  ws_ypixel;   /* vertical size, pixels (unused) */
};

#define TA_BOLD                 0x00000001 // set bold font.
#define TA_ITALIC               0x00000002 // set italic font.
#define TA_UNDLN                0x00000004 // set underline.
#define TA_UNDSCR               0x00000008 // set underscore.
#define TA_REVVID               0x00000010 // set reversre video.
#define TA_META                 0x00000020 // set meta flags.
#define TA_DISPCTRL             0x00000040 // set display control flag.
#define TA_BRTFRG               0x00000080 // set bright foreground.
#define TA_BRTBCK               0x00000100 // set bright background.
#define TA_BLINK                0x00000200 // set blink.
#define TA_DIM                  0x00000400 // set dim colors.
#define TA_UPDATED              0x00000800 // 

// tty attributes
typedef struct tty_attr {
    uint    ta_flags;   // tty attribute flags.
    int     ta_foreg;   // foreground color.
    int     ta_backg;   // background color.
    void    *ta_priv;   // device specific data.
} tty_attr_t;

/////////////////////////////////////
//    for use with tty->t_flags     |
////////////////////////////////////

#define TF_OPEN     0x001 // tty is opened.
#define TF_BUSY     0x002 // tty is busy.
#define TF_ESC      0x004 // escape is set.
#define TF_CSI      0x008 // processing CSI.
#define TF_DIRTY    0x010 // buffer is dirty.
#define TF_NOINPUT  0x020 // if set on the active TTY, input is not received by that tty.

struct tty;

typedef struct tty_ops {
    int     (*open)(struct tty *tp);
    int     (*close)(struct tty *tp);
    void    (*flush)(struct tty *tp);
    void    (*redraw_cursor)(struct tty *tp);
    void    (*erase)(struct tty *tp, off_t, usize);
    void    (*write_char)(struct tty *tp, int c);
    int     (*receive_char)(struct tty *tp, int c);
    int     (*ioctl)(struct tty *tp, int req, void *arg);
    isize   (*redraw)(struct tty *tp);
    isize   (*write)(struct tty *tp, const char *buf, usize sz);
} tty_ops_t;

typedef struct {
    ringbuf_t upbuf;
    ringbuf_t downbuf;
} tty_scroll_t;

typedef struct tty {
    device_t    *t_dev;                     // Associated device.
    unsigned    t_flags;                    // Flags.
    termios_t   t_termios;                  // Termios.
    winsize_t   t_winsize;                  // Window size for tty.

    int         t_tabsize;                  // Size of TAB character, must be a multiple of 2.

    tty_attr_t  t_attrib;                   // Current attributes.
    tty_attr_t  t_default_attrib;           // Default attributes.

    #define ESC_SEQ_COUNT   256             // Number of escape sequences.
    int         t_esc_seq[ESC_SEQ_COUNT];   // Escape sequence buffer.
    int         t_esc_seqcnt;               // Number of escape sequences in buffer.
    int         t_esc_seqpos;               // Escape sequence position

    #define TTYBUFSZ     KiB(32)
    ringbuf_t   t_inputbuf;                 // Input buffer.

    off_t       t_saved_cursor_pos;
    off_t       t_cursor_pos;               // Cursor position.

    tty_scroll_t t_scroll;
    char        *t_char_buffer;             // Character buffer.
    void        *t_shadow_buffer;
    off_t       ta_dirty_pos;
    usize       ta_dirty_size;

    tty_ops_t   t_ops;                      // Driver specific operations.
    void        *t_ldata;
    ldisc_t     *t_ldisc;                   // Line discipline.

    await_event_t   t_readers;
    await_event_t   t_writers;

    spinlock_t   t_lock;
} tty_t;

extern tty_t *active_tty;
extern tty_t *console;

#define tty_assert(tp)                  ({ assert(tp, "Invalid tty."); })
#define tty_init_lock(tp)               ({ tty_assert(tp); spinlock_init(&(tp)->t_lock); })
#define tty_lock(tp)                    ({ tty_assert(tp); spin_lock(&(tp)->t_lock); })
#define tty_unlock(tp)                  ({ tty_assert(tp); spin_unlock(&(tp)->t_lock); })
#define tty_trylock(tp)                 ({ tty_assert(tp); spin_trylock(&(tp)->t_lock); })
#define tty_assert_locked(tp)           ({ tty_assert(tp); spin_assert_locked(&(tp)->t_lock); })

#define tty_inputbuf_init(tp)           ({ tty_assert(tp); ringbuf_init(TTYBUFSZ, &(tp)->t_inputbuf); })
#define tty_inputbuf_lock(tp)           ({ tty_assert(tp); ringbuf_lock(&(tp)->t_inputbuf); })
#define tty_inputbuf_unlock(tp)         ({ tty_assert(tp); ringbuf_unlock(&(tp)->t_inputbuf); })
#define tty_inputbuf_try_lock(tp)       ({ tty_assert(tp); ringbuf_try_lock(&(tp)->t_inputbuf); })
#define tty_inputbuf_recursive_lock(tp) ({ tty_assert(tp); ringbuf_recursive_lock(&(tp)->t_inputbuf); })

extern void tty_free(tty_t *tp);
extern int  tty_alloc(tty_t **ptp);

extern int  tty_alloc_buffer(tty_t *tp);
extern void tty_free_buffers(tty_t *tp);
extern int  tty_receive_input(tty_t *tp, void *data);
extern int  tty_create(const char *name, devno_t minor, tty_ops_t *ops, ldisc_t *ldisc, tty_t **ptp);

extern void tty_switch(int tty);
extern tty_t *tty_current(void);