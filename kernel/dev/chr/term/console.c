#include <bits/errno.h>
#include <dev/cga.h>
#include <dev/uart.h>
#include <dev/tty.h>
#include <mm/kalloc.h>
#include <string.h>
#include <xytherOs_string.h>

typedef union {
        struct { uchar  c, fg : 4, bg : 4; };
        ushort attrib;
} CGA_attrib_t;

typedef struct { uint8_t r, g, b; } RGBColor;

// Define the 16 basic ANSI colors (regular + bright)
static const RGBColor ansi_colors[16] = {
    {0, 0, 0},       // rgb(0, 0, 0)        Black
    {170, 0, 0},     // rgb(170, 0, 0)      Red
    {0, 170, 0},     // rgb(0, 170, 0)      Green
    {170, 85, 0},    // rgb(170, 85, 0)     Yellow
    {0, 0, 170},     // rgb(0, 0, 170)      Blue
    {170, 0, 170},   // rgb(170, 0, 170)    Magenta
    {0, 170, 170},   // rgb(0, 170, 170)    Cyan
    {170, 170, 170}, // rgb(170, 170, 170)  White (Light Gray)
    {85, 85, 85},    // rgb(85, 85, 85)     Bright Black (Dark Gray)
    {255, 85, 85},   // rgb(255, 85, 85)    Bright Red
    {85, 255, 85},   // rgb(85, 255, 85)    Bright Green
    {255, 255, 85},  // rgb(255, 255, 85)   Bright Yellow
    {85, 85, 255},   // rgb(85, 85, 255)    Bright Blue
    {255, 85, 255},  // rgb(255, 85, 255)   Bright Magenta
    {85, 255, 255},  // rgb(85, 255, 255)   Bright Cyan
    {255, 255, 255}, // rgb(255, 255, 255)  Bright White
};

// Compute squared distance between two colors
static inline int color_distance(RGBColor a, RGBColor b) {
    int dr = (int)a.r - (int)b.r;
    int dg = (int)a.g - (int)b.g;
    int db = (int)a.b - (int)b.b;
    return dr*dr + dg*dg + db*db;
}

// returns 0-15
int rgb_to_ansi16(RGBColor rgb) {
    int best_index = 0;
    int best_distance = color_distance(rgb, ansi_colors[0]);

    for (int i = 1; i < 16; i++) {
        int d = color_distance(rgb, ansi_colors[i]);
        if (d < best_distance) {
            best_distance = d;
            best_index = i;
        }
    }
    return best_index;
}

//256-color range	Approximate 16-color
int convert_color_to_ansi(int color) {
    switch (color) {
        case  0  ...  15: return color;
        case  16 ...  21: return 1;  // Blue.
        case  22 ...  51: return 2;  // Cyan/Green.
        case  52 ...  87: return 3;  // Green/Yellow.
        case  88 ... 123: return 4;  // Yellow/Red.
        case 124 ... 159: return 5;  // Red/Magenta.
        case 160 ... 195: return 6;  // Magenta/Red.
        case 196 ... 231: return 7;  // Red.
        case 232 ... 243: return 0;  // Black/Dark Gray.
        case 244 ... 249: return 7;  // Light Gray/White.
        case 250 ... 255: return 7;  // Bright White.
        default:
            RGBColor rgb = {color & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff};
            return rgb_to_ansi16(rgb);
    }
}

static int console_open(tty_t *);
static void console_write_char(tty_t *, int);
static void console_erase(tty_t *, off_t, usize);
static void console_flush(tty_t *);
static void console_redraw_cursor(tty_t *tp);
// __unused static int console_ioctl(tty_t *, int, void *);
// __unused static isize console_read(tty_t *, char *, usize);
static isize console_write(tty_t *, const char *, usize);

static tty_ops_t console_ops = {
    .ioctl          = NULL,
    .redraw         = NULL,
    .flush          = console_flush,
    .open           = console_open,
    .write          = console_write,
    .erase          = console_erase,
    .write_char     = console_write_char,
    .redraw_cursor  = console_redraw_cursor,
};

void console_putc(int c) {
    cga_putc(c);
    uart_putc(c);
}

void console_puts(const char *s) {
    cga_puts(s);
}

static void console_flush(tty_t *tp) {
    if (tp->t_flags & TF_DIRTY) {
        ushort *vga_mem = (ushort *)V2HI(0xb8000) + tp->ta_dirty_pos;
        ushort *shadow  = (ushort *)tp->t_shadow_buffer + tp->ta_dirty_pos;

        xytherOs_memcpy(vga_mem, shadow, tp->ta_dirty_size);
        tp->t_flags &= ~TF_DIRTY;

        tp->t_ops.redraw_cursor(tp);
    }
}

static void console_redraw_cursor(tty_t *tp) {
    if (tp == NULL) return;

    const int _pos = tp->t_cursor_pos;

    outb(0x3d4, 14);
    outb(0x3d5, (((_pos >> 8) & 0xFF)));
    outb(0x3d4, 15);
    outb(0x3d5, (_pos & 0xFF));
}

static void console_write_char(tty_t *tp, int c) {
    tty_attr_t *attrib = &tp->t_attrib;
    CGA_attrib_t *cga_attrib = attrib->ta_priv;
    
    ushort *shadow = (ushort *)tp->t_shadow_buffer;

    cga_attrib->c = c;

    if (attrib->ta_flags & TA_UPDATED) {
        attrib->ta_flags &= ~TA_UPDATED;

        cga_attrib->fg = attrib->ta_flags & TA_BRTFRG ? 8 : 0;
        cga_attrib->bg = attrib->ta_flags & TA_BRTBCK ? 8 : 0;

        cga_attrib->fg |= convert_color_to_ansi(attrib->ta_foreg);
        cga_attrib->bg |= convert_color_to_ansi(attrib->ta_backg);
    }

    if (shadow[tp->t_cursor_pos] != cga_attrib->attrib) {
        tp->ta_dirty_size   = 2;
        tp->t_flags         |= TF_DIRTY;
        tp->ta_dirty_pos    = tp->t_cursor_pos;
        shadow[tp->t_cursor_pos] = cga_attrib->attrib;
    }
}

static inline void console_erase(tty_t *tp, off_t off, usize sz) {
    tty_attr_t *attrib = &tp->t_attrib;
    CGA_attrib_t *cga_attrib = attrib->ta_priv;

    ushort *shadow = (ushort *)tp->t_shadow_buffer;

    cga_attrib->c = ' ';

    if (attrib->ta_flags & TA_UPDATED) {
        attrib->ta_flags &= ~TA_UPDATED;

        cga_attrib->fg = attrib->ta_flags & TA_BRTFRG ? 8 : 0;
        cga_attrib->bg = attrib->ta_flags & TA_BRTBCK ? 8 : 0;
        cga_attrib->fg |= convert_color_to_ansi(attrib->ta_foreg);
        cga_attrib->bg |= convert_color_to_ansi(attrib->ta_backg);
    }

    memsetw(&shadow[off], cga_attrib->attrib, sz);
    tp->t_flags |= TF_DIRTY;
    tp->ta_dirty_pos = off;
    tp->ta_dirty_size = sz * 2;
}

static void console_scroll(tty_t *tp, bool down/*scroll down?*/, usize lines) {    
    const winsize_t *wsz = &tp->t_winsize;
    tty_scroll_t    *buffer = &tp->t_scroll;
    ushort          *shadow = tp->t_shadow_buffer;
    const usize     size = wsz->ws_col * (wsz->ws_row - 1) * 2;

    while (lines--) {
        if (down) {
            ringbuf_lock(&buffer->downbuf);
            ringbuf_write(
                &buffer->downbuf,
                (char *)&shadow[(wsz->ws_row - 1) * wsz->ws_col],
                wsz->ws_col * 2
            );
            ringbuf_unlock(&buffer->downbuf);

            memmove(&shadow[wsz->ws_col], shadow, size);

            ringbuf_lock(&buffer->upbuf);
            if (!ringbuf_read(&buffer->upbuf, (char *)shadow, wsz->ws_col * 2)) {
                console_erase(tp, 0, wsz->ws_col);
            }
            ringbuf_unlock(&buffer->upbuf);
        } else {
            ringbuf_lock(&buffer->upbuf);
            ringbuf_write(
                &buffer->upbuf,
                (char *)shadow,
                wsz->ws_col * 2
            );
            ringbuf_unlock(&buffer->upbuf);

            memmove(shadow, &shadow[wsz->ws_col], size);

            ringbuf_lock(&buffer->downbuf);
            if (!ringbuf_read(&buffer->downbuf, (char *)&shadow[(wsz->ws_row - 1) * wsz->ws_col], wsz->ws_col * 2)) {
                console_erase(tp, (wsz->ws_row - 1) * wsz->ws_col, wsz->ws_col);
            }
            ringbuf_unlock(&buffer->downbuf);
        }
    }

    tp->t_flags |= TF_DIRTY; // shadow buffer is dirty.
    tp->ta_dirty_pos = 0;
    tp->ta_dirty_size = wsz->ws_col * wsz->ws_row * 2;
}

static void console_update_cursor(tty_t *tp, off_t off) {
    const winsize_t *wsz = &tp->t_winsize;

    tp->t_cursor_pos = off;

    /* Handle line wrapping and scrolling */
    const usize screen_size = wsz->ws_col * wsz->ws_row;
    if (tp->t_cursor_pos >= screen_size) {
#ifdef ONOEOT
        if (tp->t_termios.c_oflag & ONOEOT) {
            /* Discard characters past screen end */
            tp->t_cursor_pos = screen_size - 1;
        } else {
            console_scroll(tp, false, 1);
            tp->t_cursor_pos = screen_size - wsz->ws_col;
        }
#else
        console_scroll(tp, false, 1);
        tp->t_cursor_pos = screen_size - wsz->ws_col;
#endif
    }

    tp->t_ops.redraw_cursor(tp);
}

static void console_set_cursor_cordinates(tty_t *tp, int x, int y) {
    const winsize_t *wsz = &tp->t_winsize;

    x = (x < 0) ? 0 : (x > wsz->ws_col) ? wsz->ws_col - 1 : x;
    y = (y < 0) ? 0 : (y > wsz->ws_row) ? wsz->ws_row - 1 : y;

    // TODO: erase cursor

    console_update_cursor(tp, y * wsz->ws_col + x);

    // TODO: print cursor
}

static void console_move_cursor(tty_t *tp, int c, int n) {
    const winsize_t *wsz = &tp->t_winsize;

    int x = tp->t_cursor_pos % wsz->ws_col;
    int y = tp->t_cursor_pos / wsz->ws_col;

    switch (c) {
        case 'A': y -= n; break; // CUU
        case 'B': y += n; break; // CUD
        case 'C': x += n; break; // CUF
        case 'D': x -= n; break; // CUB
    }

    console_set_cursor_cordinates(tp, x, y);
}

static void console_set_attributes(tty_t *tp) {
    int color_mode, color;
    tty_attr_t *attr   = &tp->t_attrib;
    const int *esc_seq = tp->t_esc_seq;

    attr->ta_flags |= TA_UPDATED; // advise the device drive attributes changed.

    for (int i = 0; i < tp->t_esc_seqcnt; ) {
        const int code = tp->t_esc_seq[i++];
        switch (code) {
            case 0:  *attr = tp->t_default_attrib;  break; // Reset all attributes

            case 1:  attr->ta_flags |= TA_BOLD;     break; // Set bold

            case 2:  attr->ta_flags |= TA_DIM;      break; // Set half-bright.

            case 3:  attr->ta_flags |= TA_ITALIC;   break; // Set italic.

            case 4:  attr->ta_flags |= TA_UNDSCR;   break; // set underscore.

            case 5:  attr->ta_flags |= TA_BLINK;    break; // set blink.

            case 7:  attr->ta_flags |= TA_REVVID;   break; // set reverse video.

            case 10: attr->ta_flags &= ~(TA_DISPCTRL | TA_META); break; // primary font.

            case 11: attr->ta_flags = (attr->ta_flags | TA_DISPCTRL) & ~TA_META;  break; // first alternate font.

            case 12: attr->ta_flags |= TA_DISPCTRL | TA_META; break; // second alternate font.

            case 21: attr->ta_flags |= TA_UNDLN;    break; // Set underline.

            case 22: attr->ta_flags &= ~(TA_BRTFRG | TA_BRTBCK); break; // set normal intensity.

            case 23: attr->ta_flags &= ~TA_ITALIC;  break; // italic off.

            case 24: attr->ta_flags &= ~TA_UNDLN;   break; // underline off.

            case 25: attr->ta_flags &= ~TA_BLINK;   break; // blink off.

            case 27: attr->ta_flags &= ~TA_REVVID;  break; // reverse video off.

            case 30 ... 37: attr->ta_foreg = code - 30; break; // Standard foreground

            case 38: case 48: // set color: code == 38: foreground | code == 48: background
                color = code == 38 ? attr->ta_foreg : attr->ta_backg;

                color_mode = tp->t_esc_seq[i++];
                if (color_mode == 2) { // set true-color
                    color = esc_seq[i + 0] << 0 |
                            esc_seq[i + 1] << 8 |
                            esc_seq[i + 2] << 16;
                    i += 3;
                } else if (color_mode == 5) { // 256-color
                    color = esc_seq[i++];
                }

                switch (code) {
                    case 38: attr->ta_foreg = color; break;
                    case 48: attr->ta_backg = color; break;
                }

                break;

            case 39: attr->ta_foreg = tp->t_default_attrib.ta_foreg; break; // Default foreground

            case 40 ... 47: attr->ta_backg = code - 40; break; // Standard background

            case 49: attr->ta_backg = tp->t_default_attrib.ta_backg; break; // set default background color

            case 90 ... 97: // Bright foreground
                attr->ta_foreg = code - 90;
                attr->ta_flags |= TA_BRTFRG;
                break;

            case 100 ... 107: // Bright background
                attr->ta_backg = code - 100;
                attr->ta_flags |= TA_BRTBCK;
                break;
        
            default: break; // Ignore unsupported codes
        }
    }
}

static void console_erase_display(tty_t *tp, int param) {
    switch (param) {
        case 0: // ESC [ 0 J: default: from cursor to end of display.
            const usize size = (tp->t_winsize.ws_row * tp->t_winsize.ws_col) - tp->t_cursor_pos;
            tp->t_ops.erase(tp, tp->t_cursor_pos, size);
            console_update_cursor(tp, tp->t_cursor_pos);
            break;
        case 1: // ESC [ 1 J: erase from start to cursor.
            tp->t_ops.erase(tp, 0, tp->t_cursor_pos);
            console_update_cursor(tp, 0);
            break;
        case 2: // ESC [ 2 J: erase whole display.
            tp->t_ops.erase(tp, 0, tp->t_winsize.ws_row * tp->t_winsize.ws_col);
            console_update_cursor(tp, 0);
            break;
        case 3: // ESC [ 3 J: erase whole display including scroll-back buffer TODO: implement this.
            break;
    }
}

static void console_erase_line(tty_t *tp, int param) {
    off_t off; usize size;
    const winsize_t *wsz = &tp->t_winsize;

    switch (param) {
        case 0: // ESC [ 0 K: default: from cursor to end of line.
            size = wsz->ws_col - (tp->t_cursor_pos % wsz->ws_col);
            tp->t_ops.erase(tp, tp->t_cursor_pos, size);
            console_update_cursor(tp, tp->t_cursor_pos);
            break;
        case 1: // ESC [ 1 K: erase from start to cursor.
            off = (tp->t_cursor_pos / wsz->ws_col) * wsz->ws_col;
            tp->t_ops.erase(tp, off, tp->t_cursor_pos - off);
            console_update_cursor(tp, off);
            break;
        case 2: // ESC [ 2 K: erase whole line.
            off = (tp->t_cursor_pos / wsz->ws_col) * wsz->ws_col;
            tp->t_ops.erase(tp, off, wsz->ws_col);
            console_update_cursor(tp, off);
            break;
    }
}

static int console_handle_csi(tty_t *tp, int c) {
    int cur_x, cur_y;
    // Default to 0 if no parameter specified
    int param0 = tp->t_esc_seqcnt > 0 ? tp->t_esc_seq[0] : 0;

    switch (c) {
        default:
            tp->t_flags &= ~(TF_ESC | TF_CSI);
            return -EINVAL; // Not a recognized CSI sequence command. 

        case 'A' ... 'D': console_move_cursor(tp, c, param0); break; // Cursor movement

        case 'E': // Cursor to next line
            cur_y = tp->t_cursor_pos / tp->t_winsize.ws_col + param0;
            console_set_cursor_cordinates(tp, 0, cur_y - 1);
            break;

        case 'F': // Cursor to previous line
            cur_y = tp->t_cursor_pos / tp->t_winsize.ws_col - param0;
            console_set_cursor_cordinates(tp, 0, cur_y - 1);
            break;

        case 'G': // Cursor horizontal absolute
            cur_x = param0 - 1;
            cur_y = tp->t_cursor_pos / tp->t_cursor_pos;
            console_set_cursor_cordinates(tp, cur_x, cur_y);
            break;

        case 'H': case 'f': // Cursor goto position
            cur_x = cur_y = 1; // default is 1
            if (tp->t_esc_seqpos >= 0) {
                cur_y = tp->t_esc_seq[0] - 1;
            }

            if (tp->t_esc_seqpos >= 1) {
                cur_x = tp->t_esc_seq[1] - 1;
            }

            console_set_cursor_cordinates(tp, cur_x, cur_y);
            break;

        case 'J': console_erase_display(tp, param0); break; // Erase screen

        case 'K': console_erase_line(tp, param0);    break; // Erase line

        case 'S': case 'T': // Scroll up/down
            const bool scroll_down = c == 'T' ? true : false;
            const usize lines = param0 > 0 ? param0 : 1;

            console_scroll(tp, scroll_down, lines);
            break;

        case 'd':           // Cursor vertical absolute
            cur_x = tp->t_cursor_pos % tp->t_winsize.ws_col;
            cur_y = param0 - 1;
            console_set_cursor_cordinates(tp, cur_x, cur_y);
            break;

        case 'h' : case 'l':
            if (tp->t_esc_seq[0] == '?' && tp->t_esc_seq[1] == 25) {
                if (c == 'h') {
                    outb(0x3d4, 0x0a);
                    uchar cursor_start = inb(0x3d5);
                    cursor_start |= 0x20;
                    outb(0x3d4, 0x0a);
                    outb(0x3d5, cursor_start);
                } else {
                    outb(0x3d4, 0x0a);
                    uchar cursor_start = inb(0x3d5);
                    cursor_start &= ~0x20;
                    outb(0x3d4, 0x0a);
                    outb(0x3d5, cursor_start);
                }
            }
            break;

        case 's': tp->t_saved_cursor_pos = tp->t_cursor_pos; break; // Save cursor position

        case 'u': 
            tp->t_cursor_pos = tp->t_saved_cursor_pos;
            tp->t_ops.redraw_cursor(tp);
            break; // Restore cursor position

        case 'm': console_set_attributes(tp); break; // Selec graphic rendition
    }

    tp->t_flags &= ~(TF_ESC | TF_CSI);
    return 0;
}

static int console_handle_esc(tty_t *tp, int c) {
    if (c == '[' && !(tp->t_flags & TF_CSI)) { // CSI sequence detected.
        tp->t_flags |= TF_CSI;
        return 0;
    }
    
    switch (c) {
        default: // most like the CSI sequence command.
            tp->t_esc_seqcnt++;
            return console_handle_csi(tp, c);

        case '0' ... '9': // Process CSI sequence.
            const int value = tp->t_esc_seq[tp->t_esc_seqpos] * 10;
            tp->t_esc_seq[tp->t_esc_seqpos] = value + c - '0';
            break;

        case '?':
            tp->t_esc_seq[tp->t_esc_seqpos++] = c;
            tp->t_esc_seqcnt++;
            break;
        case ';': // process next CSI sequence.
            if (tp->t_esc_seqpos < (int)NELEM(tp->t_esc_seq)) {
                tp->t_esc_seqcnt++;
                tp->t_esc_seqpos++;
            }
            break;
    }

    return 0;
}

/* Handle special characters */
static void console_handle_special(tty_t *tp, int c) {
    usize cursor_pos = tp->t_cursor_pos;

    switch (c) {
        case '\n':  /* Newline */
            if (tp->t_termios.c_oflag & ONLRET) {
                /* CR behavior: move to start of line */
                cursor_pos -= (tp->t_cursor_pos % tp->t_winsize.ws_col);
            }

            /* LF behavior: move down one line */
            cursor_pos += tp->t_winsize.ws_col;
            break;

        case '\r':  /* Carriage return */
            cursor_pos = tp->t_cursor_pos - (tp->t_cursor_pos % tp->t_winsize.ws_col);
            break;

        case '\t':  /* Tab */
            cursor_pos = (tp->t_cursor_pos + tp->t_tabsize) & ~(tp->t_tabsize - 1);
            break;

        case '\b':  /* Backspace */
            if (tp->t_cursor_pos > 0) {
                cursor_pos = tp->t_cursor_pos - 1;
                console_update_cursor(tp, cursor_pos);
            }
            break;

        case '\a':  /* Bell */
            /* TODO: Implement bell */
            break;

        case '\f':  /* Form feed: Clear screen */
            console_erase_display(tp, 2);
            return;

        case '\v':  /* Vertical tab */
            /* Ignored when OPOST is enabled */
            if (!(tp->t_termios.c_oflag & OPOST)) {
                tp->t_ops.write_char(tp, c);
            }
            break;

        case ' ' ... '~': /* Printable characters */
            tp->t_ops.write_char(tp, c);
            cursor_pos = tp->t_cursor_pos + 1;
            break;
        default: /* Unrecognised characters */
    }

    console_update_cursor(tp, cursor_pos);
}

static int console_open(tty_t *tp) {
    static CGA_attrib_t cga_attrib;

    tp->t_attrib.ta_flags         |= TA_UPDATED;
    tp->t_attrib.ta_priv          = &cga_attrib;
    tp->t_default_attrib.ta_priv  = &cga_attrib;
    tp->t_default_attrib.ta_flags |= TA_UPDATED;

    int err;
    usize size;

    size = tp->t_winsize.ws_row * tp->t_winsize.ws_col * 16;
    if (!tp->t_scroll.downbuf.size) {
        err = ringbuf_init(size, &tp->t_scroll.upbuf);
        if (err) {
            return err;
        }
    }

    if (!tp->t_scroll.upbuf.size) {
        err = ringbuf_init(size, &tp->t_scroll.downbuf);
        if (err) {
            ringbuf_free_buffer(&tp->t_scroll.upbuf);
            return err;
        }
    }

    if (!tp->t_char_buffer) {
        size = tp->t_winsize.ws_row * tp->t_winsize.ws_col;
        tp->t_char_buffer = kmalloc(size);
        if (!tp->t_char_buffer) {
            ringbuf_free_buffer(&tp->t_scroll.upbuf);
            ringbuf_free_buffer(&tp->t_scroll.downbuf);
            return -ENOMEM;
        }

        xytherOs_memset(tp->t_char_buffer, 0, size);
    }

    if (!tp->t_shadow_buffer) {
        size = tp->t_winsize.ws_row * tp->t_winsize.ws_col;
        tp->t_shadow_buffer = kmalloc(size * 2);
        if (!tp->t_shadow_buffer) {
            ringbuf_free_buffer(&tp->t_scroll.upbuf);
            ringbuf_free_buffer(&tp->t_scroll.downbuf);
            return -ENOMEM;
        }

        xytherOs_memsetw(tp->t_shadow_buffer, 0x0720, size);
    }

    return 0;
}

static isize console_write(tty_t *tp, const char *buf, usize count) {
    if (!tp || !buf || count == 0) {
        return -EINVAL;
    }

    isize written = 0;

    for (usize i = 0; buf[i] && (i < count); i++, written++, tp->t_ops.flush(tp)) {
        int c = buf[i];

        /* Handle escape sequences */
        if (tp->t_flags & TF_ESC) {
            int ret = console_handle_esc(tp, c);
            if (ret == 0) {
                continue;
            }

            /* If escape sequence fails, fall through to normal processing */
            tp->t_flags &= ~(TF_ESC | TF_CSI);
        }

        /* Start new escape sequence */
        if (c == '\033') {
            tp->t_flags |= TF_ESC;
            tp->t_esc_seqpos = 0;
            tp->t_esc_seqcnt = 0;
            memset(tp->t_esc_seq, 0, sizeof(tp->t_esc_seq));
            continue;
        }

        /* Apply termios output processing (if OPOST enabled) */
        if (tp->t_termios.c_oflag & OPOST) {
            /* ONOCR: Skip \r at column 0 */
            if (c == '\r' && (tp->t_termios.c_oflag & ONOCR) && 
                (tp->t_cursor_pos % tp->t_winsize.ws_col) == 0) {
                continue;
            }
        }

        console_handle_special(tp, c);
    }

    return written;
}

int console_create(int tty_minor, tty_t **ref) {
    char tty_name[32] = {0};
    
    sprintf(tty_name, "tty%d", tty_minor);
    
    tty_t *console = NULL;
    int err = tty_create(tty_name, tty_minor, &console_ops, &ldisc_X_TTY, &console);
    if (err != 0) {
        return err;
    }
    
    console->t_default_attrib.ta_backg = 0;
    console->t_default_attrib.ta_foreg = 0x7;
    console->t_attrib = console->t_default_attrib;
    
    console->t_winsize.ws_col = 80;
    console->t_winsize.ws_row = 25;
    
    err = device_register(console->t_dev);
    if (err != 0) {
        tty_free(console);
        return err;
    }

    *ref = console;

    return 0;
}

int console_init(void) {
    for (int tty = 1; tty <= 8; ++tty) {
        tty_t *tp = NULL;
        int err = console_create(tty, &tp);
        if (err) {
            return err;
        }

        err = tty_register(tty, tp);
        if (err) {
            device_unregister(&tp->t_dev->devid);
            tty_free(tp);
            return err;
        }
    }

    return 0;
}