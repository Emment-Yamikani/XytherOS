#include <bits/errno.h>
#include <dev/tty.h>
#include <mm/kalloc.h>
#include <string.h> // for memset, memcpy
#include <sys/schedule.h>
#include <sys/thread.h>

#define XTTYFBMAX 4096

typedef struct ldata {
    int     pos, len;
    char    buf[XTTYFBMAX];
    uint8_t echo:1;
    uint8_t canon:1;
    uint8_t flush:1;
} ldata_t;

static bool tty_isprint(int c) {
    return c >= 0x20 && c <= 0x7e;
}

static int x_tty_open(tty_t *tp) {
    if (tp == NULL) {
        return -EINVAL;
    }

    tty_assert_locked(tp);

    if (tp->t_ldata) {
        return -EBUSY;
    }

    ldata_t *ld = kzalloc(sizeof *ld);
    if (ld == NULL) {
        return -ENOMEM;
    }
    memset(ld, 0, sizeof(ldata_t)); // Ensure ldata is initialized.
    tp->t_ldata = ld;
    return 0;
}

static void x_tty_close(tty_t *tp) {
    if (tp == NULL || tp->t_ldata == NULL) { // Add null check for tp
        return;
    }

    tty_assert_locked(tp);

    kfree(tp->t_ldata);
    tp->t_ldata = NULL;
}

static ssize_t x_tty_read(tty_t *tp, char *buf, size_t count) {
    if (tp == NULL || buf == NULL || count == 0) {
        return -EINVAL;
    }

    tty_assert_locked(tp);    
    
    tty_inputbuf_lock(tp);
    
    ssize_t sz = 0;
    while (sz < (ssize_t)count) {
        while (ringbuf_isempty(&tp->t_inputbuf)) {
            // Wake any writers waiting for space
            await_event_signal(&tp->t_writers);

            // If we've read something, don't wait again; return partial read
            if (sz > 0) {
                tty_inputbuf_unlock(tp);
                return sz;
            }

            tty_unlock(tp); // unlock tty.

            // Wait for readers; release lock while sleeping
            ssize_t ret = await_event(&tp->t_readers, &tp->t_inputbuf.lock);
            
            tty_lock(tp); // lock tty.
            
            if (ret) {
                tty_inputbuf_unlock(tp);
                return ret;
            }

            // When we wake up, we need to recheck the buffer
        }

        // Attempt to read remaining bytes
        size_t to_read = count - sz;
        size_t n = ringbuf_read(&tp->t_inputbuf, buf + sz, to_read);

        sz += n;

        // In canonical mode, stop at newline
        if ((tp->t_termios.c_lflag & ICANON) && sz > 0 && buf[sz - 1] == '\n') {
            break;
        }

        // In non-canonical mode, just keep reading until count is filled or no more data
        if (!(tp->t_termios.c_lflag & ICANON) && n == 0) {
            break; // Avoid infinite loop if ringbuf_read returns 0
        }
    }

    // Wake up any writers who may be waiting for space
    await_event_signal(&tp->t_writers);

    tty_inputbuf_unlock(tp);
    return sz;
}

static int x_tty_process_esc(tty_t *tp, const char *seq, int maxlen) {
    if (!tp || !tp->t_ldata || !seq || maxlen < 2)
        return 0;

    ldata_t *ld = tp->t_ldata;

    if (seq[0] != '[')
        return 1; // Consume just ESC

    char cmd = seq[1];
    int consumed = 2;

    switch (cmd) {
        case 'C': // Cursor Right
            if (ld->pos < ld->len) {
                ld->pos++;
                if (tp->t_termios.c_lflag & ECHO) {
                    tp->t_ops.write(tp, "\033[1C", 4); // Echo right arrow
                }
            }
            break;

        case 'D': // Cursor Left
            if (ld->pos > 0) {
                ld->pos--;
                if (tp->t_termios.c_lflag & ECHO) {
                    tp->t_ops.write(tp, "\033[1D", 4); // Echo left arrow
                }
            }
            break;
            
        default:
            break; // Unknown sequence — consume it anyway
    }

    return consumed;
}

static void tty_echo(tty_t *tp, int c) {
    ldata_t   *ld   = tp->t_ldata;
    termios_t *tio  = &tp->t_termios;

    bool echoctl    = false;
    bool echoc      = tio->c_lflag & ECHO ? true : false;
    bool isNL       = c == '\n' || c == tio->c_cc[VEOL] || c == tio->c_cc[VEOL2];
    bool isspecial  = c == '\a' || c == '\b' || c == '\t' || c == '\n' || c == '\r' || c == '\e';

#ifdef ECHOCTL
    echoctl = tio->c_lflag & ECHOCTL ? true : false;
#endif

    if (echoc && tty_isprint(c)) {
        if (ld->pos == ld->len) {
            tp->t_ops.write(tp, (char *)&c, 1);
        } else if (ld->pos < ld->len) {
            int delta = ld->len - ld->pos + 1;

            char out[delta + 16];
            char esc[] = "\033[K\033[s%s\033[u\033[1C";
            /// erase from cursor to end of line
            /// save current cursor position
            /// write to the screen
            /// restore cursor position
            /// move screen cursor to the right
            off_t len = sprintf(out, esc, ld->buf + ld->pos - 1);
            tp->t_ops.write(tp, out, len);
        }
    } else if (echoc && echoctl && (c < 0x20) && !isspecial) {
        char ctl[] = {'^', c + 0x40};
        tp->t_ops.write(tp, ctl, 2);
    } else if (isNL && tio->c_lflag & ECHONL) {
        tp->t_ops.write(tp, "\n", 1);
    }
}

static void tty_insert(tty_t *tp, int c) {
    if (!tp || !tp->t_ldata) return; // Add null check for tp
    ldata_t *ld = tp->t_ldata;

    if (ld->pos < XTTYFBMAX -3) { // check if there is space
        if (c != '\n' && ld->pos < ld->len) {
            char *buf = ld->buf;
            int delta = ld->len - ld->pos;
            memmove(buf + ld->pos + 1, buf + ld->pos, delta);
        }

        if (c == '\n') {
            ld->buf[ld->len++] = c; // NL always inserted at EOL
        } else {
            ld->buf[ld->pos++] = c;
            ld->len++;
        }
    }

    if (ld->pos == XTTYFBMAX -3) { // changed to XTTYFBMAX -3
        ld->flush = true;
    }
}

static int x_tty_receive_buf(tty_t *tp, const char *cp, char *fp __unused, int count) {
    if (!tp || !tp->t_ldata) {
        return -EINVAL;
    }

    tty_assert_locked(tp);

    ldata_t     *ld = tp->t_ldata;
    termios_t   *tio = &tp->t_termios;

    for (int i = 0; i < count && cp[i]; i++) {
        char c = cp[i];

        if (tio->c_lflag & ISIG) {
            if (c == tio->c_cc[VINTR]) {
                // Handle interrupt signal (e.g., send SIGINT to process group)
                // For simplicity, we'll just flush the buffer here.
                ld->pos = 0;
                ld->len = 0;
                // if (tp->t_ops.signal)
                    // tp->t_ops.signal(tp, SIGINT);
                continue;
            } else if (c == tio->c_cc[VQUIT]) {
                // Handle quit signal (e.g., send SIGQUIT to process group)
                ld->pos = 0;
                ld->len = 0;
                // if (tp->t_ops.signal)
                    // tp->t_ops.signal(tp, SIGQUIT);
                continue;
            } else if (c == tio->c_cc[VSUSP]) {
                // Handle suspend signal (e.g., send SIGTSTP to process group)
                ld->pos = 0;
                ld->len = 0;
                // if (tp->t_ops.signal)
                //   tp->t_ops.signal(tp, SIGTSTP);
                continue;
            }
        }

        if (!(tio->c_lflag & ICANON)) {
            tty_inputbuf_lock(tp);
            ringbuf_write(&tp->t_inputbuf, &c, 1);
            await_event_signal(&tp->t_readers);
            tty_inputbuf_unlock(tp);
            continue;
        }

        if (c == '\b' || c == tio->c_cc[VERASE]) {
            if (ld->pos > 0) {
                ld->pos--;
                if (ld->pos < (ld->len - 1)) {
                    int delta = ld->len - ld->pos;
                    memmove(ld->buf + ld->pos, ld->buf + ld->pos + 1, delta - 1);
                    ld->buf[ld->pos + (delta - 1)] = '\0';

                    if (tio->c_lflag & ECHO) {
                        char out[delta + 16];
                        char esc[] = "\033[1D\033[K\033[s%s\033[u";
                        /// move screen cursor to the left
                        /// erase from cursor to end of line
                        /// save current cursor position
                        off_t len = sprintf(out, esc, ld->buf + ld->pos);
                        tp->t_ops.write(tp, out, len);
                    }
                } else if (ld->pos == (ld->len - 1)) {
                    ld->buf[ld->len] = '\0';
                    if (tio->c_lflag & ECHO) {
                        tp->t_ops.write(tp, "\b \b", 3);
                    }
                }
                ld->len--;
            }
            continue;
        } else if (c == tio->c_cc[VKILL]) {
            if (tio->c_lflag & ECHOK) {
                for (int j = 0; j < ld->pos; j++)
                    tp->t_ops.write(tp, "\b \b", 3);
            }

            memset(ld->buf, 0, ld->len);
            ld->pos = 0;
            ld->len = 0;
            continue;
        } else if (c == tio->c_cc[VEOF]) {
            if (ld->len == 0){
                tty_inputbuf_lock(tp);
                ringbuf_write(&tp->t_inputbuf, &c, 1);
                await_event_signal(&tp->t_readers);
                tty_inputbuf_unlock(tp);
            }
            continue;
        } else if (c == tio->c_cc[VWERASE]) {
            // Erase the previous word
            int k = ld->pos - 1;
            while (k >= 0 && (ld->buf[k] == ' ' || !tty_isprint(ld->buf[k]))) {
                k--;
            }
            while (k >= 0 && tty_isprint(ld->buf[k])) {
                k--;
            }
            int chars_to_erase = ld->pos - (k + 1);
            if (tio->c_lflag & ECHOE && chars_to_erase > 0) {
                 for(int j = 0; j < chars_to_erase; j++){
                    tp->t_ops.write(tp, "\b \b", 3);
                 }
            }
            ld->pos = k + 1;
            ld->len = k + 1;
            continue;

        } else if (c == tio->c_cc[VDISCARD]) {
            // Discard all buffered output

            memset(ld->buf, 0, ld->len);
            ld->pos = 0;
            ld->len = 0;
            if (tio->c_lflag & ECHOK)
                tp->t_ops.write(tp, "\n", 1);
            continue;
        }

        if (c == '\r' && tio->c_iflag & IGNCR) {
            continue;
        } else if (c == '\r' && tio->c_iflag & ICRNL) {
            c = '\n';
        } else if (c == '\n' && tio->c_iflag & INLCR) {
            tty_insert(tp, '\r');
        } else if (c == '\e') {
            int remaining = count - i - 1;
            if (remaining <= 0) break;

            const char *esc_seq = &cp[i + 1];
            int consumed = x_tty_process_esc(tp, esc_seq, remaining);
            i += consumed; // skip ESC + sequence
            continue;
        }

        bool isNL = c == '\n' || c == tio->c_cc[VEOL] || c == tio->c_cc[VEOL2];

        if (tty_isprint(c) || isNL) {
            tty_insert(tp, c);
        }

        tty_echo(tp, c);

        if (isNL || ld->flush) {
            tty_inputbuf_lock(tp);
            ringbuf_write(&tp->t_inputbuf, ld->buf, ld->len);
            await_event_signal(&tp->t_readers);
            tty_inputbuf_unlock(tp);

            memset(ld->buf, 0, ld->len);
            ld->pos = 0;  // Reset buffer position and length after writing to cooked buffer
            ld->len = 0;
            continue;
        }

        // TODO: VTIME
        // TODO: VMIN
        // TODO: VSWTC
        // TODO: VSTART
        // TODO: VSTOP
        // TODO: VREPRINT
        // TODO: VLNEXT
    }

    return 0;
}

void x_tty_flush_buffer(tty_t *tp) {
    if (!tp || !tp->t_ldata) return;

    ldata_t *ld = tp->t_ldata;

    memset(ld->buf, 0, ld->len);

    ld->pos = 0;
    ld->len = 0;
    ld->flush = 1;
}

ssize_t x_tty_chars_in_buffer(tty_t *tp) {
    if (!tp || !tp->t_ldata) return 0;
    ldata_t *ld = tp->t_ldata;
    return ld->len;
}

ssize_t x_tty_write(tty_t *tp, const char *buf, size_t nr) {
    if (!tp || !tp->t_ops.write) { // Check for null t_ops
        return -EINVAL;
    }

    tty_assert_locked(tp);
    return tp->t_ops.write(tp, buf, nr);
}

int x_tty_ioctl(tty_t *tp, uint cmd, void *arg);

void x_tty_set_termios(tty_t *tp, struct termios *old);

ldisc_t ldisc_X_TTY = {
    .name        = "x_TTY",
    .magic       = 0x797474,
    .open        = x_tty_open,
    .close       = x_tty_close,
    .read        = x_tty_read,
    .receive_buf = x_tty_receive_buf,
    .flush_buffer = x_tty_flush_buffer,
    .chars_in_buffer = x_tty_chars_in_buffer,
    .write      = x_tty_write,
};