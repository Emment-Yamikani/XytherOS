#include <xyther/stdio.h>
#include <xyther/unistd.h>

int __stdin_fileno = -1, __stdout_fileno = -1, __stderr_fileno = -1;

long __std_in(char *buf, unsigned long len) {
    return read(__stdin_fileno, buf, len);
}

long __std_out(char *buf, unsigned long len) {
    return write(__stdout_fileno, buf, len);
}

long __std_err(char *buf, unsigned long len) {
    return write(__stderr_fileno, buf, len);
}
