#include <xyther/unistd.h>
#include <xyther/string.h>
#include <xyther/stdio.h>

extern long write_tty(const char *str, size_t len);

void clear_tty(void) {
    write_tty("\e[2J", 5);
}

int open_tty(void) {
    if ((__stdin_fileno = open("/dev/tty1", O_RDONLY, 0)) < 0) {
        return __stdin_fileno;
    }

    if ((__stdout_fileno = open("/dev/tty1", O_WRONLY, 0)) < 0) {
        close(__stdin_fileno);
        return __stdout_fileno;
    }

    if ((__stderr_fileno = open("/dev/tty1", O_WRONLY, 0)) < 0) {
        close(__stdout_fileno);
        close(__stdin_fileno);
        return __stderr_fileno;
    }

    clear_tty();
    return 0;
}

void close_tty(void) {
    close(__stderr_fileno);
    close(__stdout_fileno);
    close(__stdin_fileno);
}

long write_tty(const char *str, size_t len) {
    return write(__stdout_fileno, str, len);
}

long read_tty(char *buf, size_t len) {
    long count = read(__stdin_fileno, buf, len);
    buf[count  < len ? count : len ] = '\0';
    return count;
}

size_t __strlen(const char *str) {
    char *s = (char *)str;
    while (*s++);
    return s - str;
}

long print(const char *str) {
    size_t len = __strlen(str);
    return write_tty(str, len);
}

long println(char *a) {
    long len = print(a);
    len += print("\n");
    return len;
}

void print_char(int ch) {
    write_tty((const char *)&ch, 1);
}

void get_input(char *buf, size_t len) {
    read_tty(buf, len);
}

int main(void) {
    open_tty();
    
    printf("Hello %d.%d.%d\n", 1, 9, 1);

    return 0;
}