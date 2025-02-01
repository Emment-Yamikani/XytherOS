#include <core/defs.h>
#include <core/types.h>
#include <dev/cga.h>
#include <arch/x86_64/asm.h>
#include <string.h>

static int      cga_pos     = 0;
static uint8_t  cga_attr    = 0;
static uint16_t *cga_addr   = 0;

void cga_setcolor(int back, int fore) {
    cga_attr = (back << 4) | fore;
}

void cga_setcursor(int pos) {
    outb(0x3d4, 14);
    outb(0x3d5, (AND(SHR(pos, 8), 0xFF)));
    outb(0x3d4, 15);
    outb(0x3d5, (pos & 0xFF));
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

static int cga_putchar(const int c) {
    static volatile bool cga_active = 0;

    if (!cga_active) {
        cga_init();
        cga_active = 1;
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