#include <xyther/stdio.h>
#include <xyther/unistd.h>

void __clear_stdio(void) {
    // __std_out("\e[?25l\e[2J", 11);
}

int __open_stdio(void) {
    int err = open("/dev/tty1", O_RDONLY, 0);
    if (err != 0) { return err; }

    err = open("/dev/tty1", O_WRONLY, 0); 
    if (err < 0) {
        close(__stdin_fileno);
        return err;
    }

    err = open("/dev/tty1", O_WRONLY, 0);
    if (err < 0) {
        close(__stdout_fileno);
        close(__stdin_fileno);
        return err;
    }

    __clear_stdio();
    return 0;
}

void __close_stdio(void) {
    close(__stderr_fileno);
    close(__stdout_fileno);
    close(__stdin_fileno);
}

long __std_in(char *buf, unsigned long len) {
    return read(__stdin_fileno, buf, len);
}

extern void kputc(int);

unsigned long kernel_putstr(const char *str, unsigned long len) {
    unsigned long index = 0;
    while (index < len && str[index] != '\0') kputc(str[index++]);
    return index;
}

long __std_out(char *buf, unsigned long len) {
    // return write(__stdout_fileno, buf, len);
    return (long)kernel_putstr(buf, len);
}

long __std_err(char *buf, unsigned long len) {
    // return write(__stderr_fileno, buf, len);
    return (long)kernel_putstr(buf, len);
}
