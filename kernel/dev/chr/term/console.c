#include <dev/cga.h>
#include <dev/uart.h>

void console_putc(int c) {
    cga_putc(c);
    uart_putc(c);
}

void console_puts(const char *s) {
    cga_puts(s);
}