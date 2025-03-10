#include <arch/x86_64/asm.h>
#include <core/defs.h>
#include <core/types.h>
#include <dev/cga.h>
#include <string.h>
#include <lib/ctype.h>
#include <mm/mem.h>
#include <ds/stack.h>

static int      cga_pos     = 0;
static uint8_t  cga_attr    = 0;
static uint16_t *cga_addr   = 0;
static int      cga_esc     = 0;
static stack_t  *cga_chars  = STACK_NEW();
static stack_t  *cga_themes = STACK_NEW();

void cga_setcolor(int back, int fore) {
    cga_attr = (back << 4) | fore;
}

void cga_setcursor(int __pos) {
    outb(0x3d4, 14);
    outb(0x3d5, (AND(SHR(__pos, 8), 0xFF)));
    outb(0x3d4, 15);
    outb(0x3d5, (__pos & 0xFF));
}

void cga_scroll(void) {
    memmove(cga_addr, &cga_addr[80], 2 * ((80 * 25) - 80));
    cga_pos -= 80;
    memsetw(&cga_addr[80 * 24], ((uint16_t)cga_attr << 8) | (' '), 80);
}

void cga_clr(void) {
    memsetw(&cga_addr[0], (uint16_t)cga_attr << 8, (25 * 80));
    cga_setcursor(cga_pos = 0);
}

int cga_init(void) {
    cga_addr = (uint16_t *)V2HI(0xb8000);
    cga_setcolor(CGA_BLACK, CGA_WHITE);
    cga_clr();
    return 0;
}

// static int cga_putchar(const int c) {
//     static volatile bool cga_active = 0;

//     if (!cga_active) {
//         cga_init();
//         cga_active = 1;
//     }

//     if (c == '\n')
//         cga_pos += 80 - cga_pos % 80;
//     else if (c == '\b')
//         cga_addr[--cga_pos] = ((cga_attr << 8) & 0xff00) | (' ' & 0xff);
//     else if (c == '\t')
//         cga_pos = (cga_pos + 4) & ~3;
//     else if (c == '\r')
//         cga_pos = (cga_pos / 80) * 80;
//     if (c >= ' ')
//         cga_addr[cga_pos++] = ((cga_attr << 8) & 0xff00) | (c & 0xff);
//     /*cons_scroll up*/
//     if ((cga_pos / 80) > 24)
//         cga_scroll();
//     cga_setcursor(cga_pos);
//     return 0;
// }


static int cga_putchar(const int c) {
    static volatile bool cga_active = 0;

    if (!cga_active) {
        cga_init();
        cga_active = 1;
    }

    if (c == '\e') {
        cga_esc = 1;
        return 0;
    }

    if (vmm_active()) {

        if (cga_esc) {
            long val = 0;
            static int open_ = 0;
            static int16_t fg = 0;
            static int16_t bg = 0;
            if (c == '[') {
                open_ = 1;
                val = cga_attr;
                stack_lock(cga_themes);
                stack_push(cga_themes, (void *)val);
                stack_unlock(cga_themes);
                return 0;
            }

            if (open_) {
                if (c == ';') {
                    open_ = 0;
                    stack_lock(cga_chars);
                    for (int ni = 0, i = 0, pw = 1;
                        stack_pop(cga_chars, (void **)((long *)&c)) == 0; ni++) {
                        for (pw = 1, i = ni; i; --i)
                            pw *= 8;
                        bg += c * pw;
                    }
                    stack_unlock(cga_chars);
                    return 0;
                }

                if (c == 'm') {
                    stack_lock(cga_themes);
                    stack_pop(cga_themes, (void **)&val);
                    stack_pop(cga_themes, (void **)&val);
                    stack_unlock(cga_themes);
                    cga_attr = val;
                    fg = 0;
                    bg = 0;
                    open_ = 0;
                    cga_esc = 0;
                    return 0;
                }

                if (isdigit(c)) {
                    stack_lock(cga_chars);
                    stack_push(cga_chars, (void *)(long)(c - '0'));
                    stack_unlock(cga_chars);
                }
                return 0;
            }

            if (c == 'm') {
                stack_lock(cga_chars);
                for (int ni = 0, i = 0, pw = 1;
                    stack_pop(cga_chars, (void **)((long *)&c)) == 0; ni++) {
                    for (pw = 1, i = ni; i; --i)
                        pw *= 8;
                    fg += c * pw;
                }
                stack_unlock(cga_chars);
                cga_setcolor(bg, fg);
                fg = 0;
                bg = 0;
                cga_esc = 0;
                return 0;
            }
            if (isdigit(c)) {
                stack_lock(cga_chars);
                stack_push(cga_chars, (void *)(long)(c - '0'));
                stack_unlock(cga_chars);
            }
            return 0;
        }
    } else {
        if (cga_esc) {
            if (c == 'm')
                cga_esc = 0;
            return 0;
        }
    }

    if (c == '\n')
        cga_pos += 80 - cga_pos % 80;
    else if (c == '\b')
        cga_addr[--cga_pos] = ((cga_attr << 8) & 0xff00) | (' ' & 0xff);
    else if (c == '\t')
        cga_pos = (cga_pos + 4) & ~3;
    else if (c == '\r')
        cga_pos = (cga_pos / 80) * 80;
    if (c >= ' ')
        cga_addr[cga_pos++] = ((cga_attr << 8) & 0xff00) | (c & 0xff);
    /*cons_scroll up*/
    if ((cga_pos / 80) > 24)
        cga_scroll();
    cga_setcursor(cga_pos);
    return 0;
}


void cga_putc(const int c) {
    cga_putchar(c);
}

size_t cga_puts(const char *s) {
    char *S = (char *)s;
    while (S && *S)
        cga_putchar(*S++);
    return (size_t)(S - s);
}