#include <xyther/unistd.h>
#include <xyther/string.h>
#include <xyther/stdio.h>

#define loop() while (1)

int main(int argc, char *argv[]) {
    int err = __open_stdio();
    if (err != 0) { return err; }

    printf("[%d:%d]: Hello, User land.\n", getpid(), gettid());
    printf("This is init.\n");

    err = chroot("/ramfs/");
    if (err != 0) {
        printf("chdir: failed because of error: %d\n", err);
        goto error;
    }

    pid_t pid = err = fork();
    if (pid < 0) { // Error
        printf("fork(%d): Failed to fork child process.\n", err);
        goto error;
    } else if (pid == 0) { // Child
        printf("Hello from child.\n");
        timespec_t ts = {2, 0};
        nanosleep(&ts, NULL);
        printf("Child slept\n");
        char *const envp[] = {NULL};
        char *const argp[] = {"/shell", "--script=/shell.script", "--user=root", NULL};
        err = execve(*argp, argp, envp);
        printf("execve(%d): Failed to run '%s'.\n", err, *argp);
        goto error;
    } else { // Parent
        printf("Child %d forked\n", pid);
        pid = err = waitpid(-1, 0, 0);
        if (err < 0) {
            printf("waitpid(%d): Failed to wait for child to change state.\n", err);
            goto error;
        }
    }

    loop();
error:
    __close_stdio();
    exit(err);
    __builtin_unreachable();
}