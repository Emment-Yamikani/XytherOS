#include <dev/cga.h>

void console_putc(int c) {
    cga_putc(c);
}