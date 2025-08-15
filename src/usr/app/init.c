#include <xyther/unistd.h>
#include <xyther/string.h>
#include <xyther/stdio.h>

extern long write_tty(const char *str, size_t len);

void clear_tty(void) {
    write_tty("\e[?25l\e[2J", 11);
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

long println(char *a) {
    return printf("%s\n", a);
}

void get_input(char *buf, size_t len) {
    read_tty(buf, len);
}

void *hello(void *) {

    printf("Hello thread.\n");

    return NULL;
}

void print_status(int retval) {
    printf("%s\n", !retval ? "Success" : "Failure");
}

int main(void) {
    open_tty();
    
    pid_t pid = fork();

    if (pid < 0) {
        printf("Error forking new process\n");
        return 1;
    } else if (pid == 0) { // Child process executes this.
        printf("pid[%d]: I am the child\n", getpid());
    } else { // Parent process executed this.
        const timespec_t ts = (const timespec_t) {1, 0};
        nanosleep(&ts, NULL);

        printf("pid[%d]: I am the parent\n", getpid());
    }

    while (1) {
    }

    return 0;
}