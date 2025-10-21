#include <bits/errno.h>
#include <core/debug.h>
#include <dev/dev.h>
#include <limits.h>
#include <mm/kalloc.h>
#include <mm/mem.h>
#include <sync/atomic.h>
#include <sys/thread.h>
#include <xytherOs_string.h>

#define MAX_CMD_LEN     8192
#define MAX_ARGS        256
#define HISTORY_SIZE    128

atomic_int shell_tid    = 0;
atomic_int shell_exit   = true;
atomic_int shell_dead   = true;

/* Shell command history */
static char *cmd_history[HISTORY_SIZE];
static int history_index = 0;
static int history_count = 0;

typedef struct kshell_cmd {
    size_t  argc;
    char    **args;
} kshell_cmd_t;

static SPINLOCK(shell_lk);

#define shell_lock()            ({ spin_lock(shell_lk); })
#define shell_unlock()          ({ spin_unlock(shell_lk); })
#define shell_try_lock()        ({ spin_trylock(shell_lk); })
#define shell_islocked()        ({ spin_islocked(shell_lk); })
#define shell_recursive_lock()  ({ spin_recursive_lock(shell_lk); })
#define shell_assert_locked()   ({ spin_assert_locked(shell_lk); })

static void *kernel_shell(void *);

/* TTY device handling */
static devid_t *tty = DEVID_PTR(NULL, CHRDEV, DEV_T(TTY_DEV_MAJOR, 0));

void tty_write(const char *buf) {
    device_write(tty, 0, (char *)buf, strlen(buf));
}

void tty_open(void) {
    if (!device_open(tty, NULL)) {
        tty_write("\033[2J\033[H\033[?25l"); /* Clear screen and move cursor home */
    }
}

void tty_read(char *buf, size_t nb) {
    ssize_t len = device_read(tty, 0, buf, nb);
    if (len > 0) buf[len - 1] = '\0'; /* Null-terminate and remove newline */
}

/* Shell utilities */
static void print_prompt(void) {
    char prompt[64];
    int tid = gettid();
    snprintf(prompt, sizeof(prompt) - 1, "[\e[32mxytherOS:%d\e[m $] ", tid);
    tty_write(prompt);
}

/**
 * Read a line from cooked TTY input
 * (Simplified version that expects the TTY driver to handle line editing)
 */
static void get_user_input(char *buf, size_t size) {
    ssize_t len;
    
    if (size == 0) return;
    
    /* Read until we get a complete line */
    while (1) {
        len = device_read(tty, 0, buf, size - 1);
        if (len <= 0) continue;
        
        /* Null-terminate and strip newline */
        buf[len] = '\0';
        char *newline = __xytherOs_strchr(buf, '\n');
        if (newline) *newline = '\0';
        
        /* Return if we got non-whitespace input */
        for (char *p = buf; *p; p++) {
            if (*p != ' ' && *p != '\t') {
                return;
            }
        }
        
        /* If we get here, it was all whitespace - show prompt again */
        print_prompt();
    }
}

static void add_to_history(const char *command) {
    const int index = history_index - (history_index ? 1 : 0);
    if (history_count > 0 && xytherOs_string_eq(command, cmd_history[index % HISTORY_SIZE])) {
        return;  /* Don't add duplicate consecutive commands */
    }

    if (cmd_history[history_index % HISTORY_SIZE]) {
        kfree(cmd_history[history_index % HISTORY_SIZE]);
    }

    cmd_history[history_index % HISTORY_SIZE] = xytherOs_strdup(command);
    history_index = (history_index + 1) % HISTORY_SIZE;
    if (history_count < HISTORY_SIZE) history_count++;
}

static void free_history(void) {
    for (size_t cmd_index = 0; cmd_index < HISTORY_SIZE; cmd_index++) {
        void *command = cmd_history[cmd_index];
        if (command) {
            kfree(command);
        }
        cmd_history[cmd_index] = NULL;
    }

    history_index = 0;
    history_count = 0;
}

static void show_history(void) {
    char buf[128];
    int start = (history_index - history_count + HISTORY_SIZE) % HISTORY_SIZE;
    
    for (int i = 0; i < history_count; i++) {
        int idx = (start + i) % HISTORY_SIZE;
        snprintf(buf, sizeof(buf) - 1, "%d: %s\n", i+1, cmd_history[idx]);
        tty_write(buf);
    }
}

/* Command implementations */
static void cmd_show(char **args) {
    if (!args[1]) {
        tty_write("Usage: show [mem|threads|tasks|devices]\n");
        return;
    }

    for (int i = 1; args[i]; i++) {
        if (xytherOs_string_eq("mem", args[i]) || xytherOs_string_eq("m", args[i])) {
            mem_stats_t stats;
            getmemstats(&stats);
            
            char buf[256];
            snprintf(buf, sizeof(buf),
                "Memory Stats: Total: %zu MiB | Used:  %zu MiB | Free:  %zu MiB\n",
                stats.total / 1024, stats.used / 1024, stats.free / 1024);
            tty_write(buf);
        } else if (xytherOs_string_eq("threads", args[i])) {
            thread_t *thread;

            tty_write("/------------------------------------------------------\\\n"
                      "| PID | TID | TGID | KTID |  FLAGS  |  STATUS  |  EXIT |\n"
                      "|------------------------------------------------------|\n");
            bool sgr = false;
            queue_lock(global_thread_queue);
            foreach_thread(global_thread_queue, thread, t_global_qnode) {
                char out[2048] = {0};
                thread_info_t *ti = &thread->t_info;
                const char *color = (sgr ^= true) ? "\e[47;100m" : "\e[40;37m";

                thread_lock(thread);
                size_t len = snprintf(out, sizeof out - 1,
                    "|%s %3.3d \e[0m|%s %3.3d \e[0m|%s %4.3d \e[0m|%s %4.3d \e[0m|%s %7.7x \e[0m|%s %8s \e[0m|%s %6.4x\e[0m|\n",
                    color, thread_getpid(thread), color, ti->ti_tid, color, ti->ti_tgid, color, ti->ti_ktid,
                    color, ti->ti_flags, color, tget_state(ti->ti_state), color, ti->ti_exit
                );
                thread_unlock(thread);
                device_write(tty, 0, out, len);
            }
            queue_unlock(global_thread_queue);

            tty_write(" \\-----------------------------------------------------/\n");
        } else if (xytherOs_string_eq("thread", args[i]) || xytherOs_string_eq("t", args[i])) {
            tid_t tid = __xytherOs_atoi(args[i + 1]);
            if (tid > 0) {
                thread_info_t ti;
                int err = thread_get_info_by_id(tid, &ti);
                if (err == 0) {
                    thread_info_dump(&ti);
                }
            }
        } else if (xytherOs_string_eq("devices", args[i]) || xytherOs_string_eq("d", args[i])) {
            // device_dump_all();
        }
    }
}

static void cmd_help(void) {
    tty_write("xytherOS Kernel Shell Commands:\n"
        "  clear                      - Clear the screen\n"
        "  echo [args...]             - Echo the arguments\n"
        "  halt [thread]              - Stop execution\n"
        "  help                       - Show this help\n"
        "  history                    - Show command history\n"
        "  kill <tid>                 - Terminate a thread\n"
        "  run <name> [args...]       - Start a new thread\n"
        "  show [mem|threads|devices] - Display system information\n"
    );
}

static void cmd_echo(char **args) {
    foreach(arg, &args[1]) {
        tty_write(arg);
        tty_write(" ");
    }

    tty_write("\n"); // Add a NL.
}

static void cmd_clear(void) {
    tty_write("\033[2J\033[H");
}

static void cmd_kill(char **args) {
    if (!args[1]) {
        tty_write("Usage: kill <thread_id>\n");
        return;
    }

    int tid = __xytherOs_atoi(args[1]);
    if (tid <= 0) {
        tty_write("Invalid thread ID\n");
        return;
    }

    int err = pthread_kill(tid, SIGKILL);
    if (err == 0) {
        tty_write("Thread terminated successfully\n");
    } else {
        tty_write("Failed to terminate thread\n");
    }
}

static void cmd_run(char **args) {
    if (!args[1]) {
        tty_write("Usage: run <name> [args...]\n");
        return;
    }

    int     argc   = 0;
    char    *name  = args[1];
    char    **argv = &args[1];

    while (argv[argc]) argc++;

    int tid = thread_create(NULL, NULL, NULL, THREAD_CREATE_SCHED, NULL);
    if (tid > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Started thread '%s' with TID %d\n", name, tid);
        tty_write(buf);
    } else {
        tty_write("Failed to start thread\n");
    }
}

static void cmd_exit(void) {
    atomic_store(&shell_exit, true);
}

int get_command(kshell_cmd_t *command) {
    if (command == NULL) {
        return -EINVAL;
    }

    char *buf = (char *)kzalloc(MAX_CMD_LEN);
    if (buf == NULL) {
        return -ENOMEM;
    }

    char **cmd_tokens = kcalloc(MAX_ARGS + 1, sizeof (char *));
    if (cmd_tokens == NULL) {
        kfree(buf);
        return -ENOMEM;
    }

    get_user_input(buf, MAX_CMD_LEN - 1);

    size_t token_count;
    int err = __xytherOs_tokenize_r(buf, "\t\r\n ", cmd_tokens, MAX_ARGS, &token_count);
    if (err != 0) {
        goto error;
    }
    cmd_tokens[token_count] = NULL; /* Ensure NULL termination */

    if (token_count <= 0) {
        err = -EINVAL;
        goto error;
    }

    command->argc = token_count;
    command->args = cmd_tokens;
    
    kfree(buf);

    return 0;
error:
    kfree(cmd_tokens);
    kfree(buf);
    return err;
}

/* Main command processor */
int process_command(kshell_cmd_t *command) {
    if (command == NULL) {
        return -EINVAL;
    }

    add_to_history(command->args[0]);

    if (xytherOs_string_eq("show", command->args[0])) {
        cmd_show(command->args);
    } else if (xytherOs_string_eq("help", command->args[0])) {
        cmd_help();
    } else if (xytherOs_string_eq("echo", command->args[0])) {
        cmd_echo(command->args);
    } else if (xytherOs_string_eq("exit", command->args[0])) {
        cmd_exit();
    } else if (xytherOs_string_eq("clear", command->args[0])) {
        cmd_clear();
    } else if (xytherOs_string_eq("kill", command->args[0])) {
        cmd_kill(command->args);
    } else if (xytherOs_string_eq("run", command->args[0])) {
        cmd_run(command->args);
    } else if (xytherOs_string_eq("history", command->args[0])) {
        show_history();
    } else {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg) - 1, "%s: command not found.\n", command->args[0]);
        tty_write(err_msg);
    }

    /* Free tokens */
    foreach(arg, command->args) {
        kfree(arg);
    }

    kfree(command->args);
    
    return 0;
}

/* Shell thread main function */
static void *kernel_shell(void *) {
    atomic_store(&shell_tid, gettid());

    tty_open();
    tty_write("xytherOS Kernel Shell\nType 'help' for available commands.\n");

    loop() {
        shell_lock();
        if (atomic_load(&shell_exit) == true) {
            free_history();

            atomic_store(&shell_tid, 0);
            atomic_store(&shell_dead, true);
            shell_unlock();
            break;
        }
        
        print_prompt();

        int err = 0;
        kshell_cmd_t command = {0};
        if ((err = get_command(&command))) {
            tty_write("Error getting command.\n");
            cmd_help();
            shell_unlock();
            continue;
        }

        if ((err = process_command(&command))) {
            tty_write("Error processing command.\n");
            cmd_help();
        }

        shell_unlock();
    }

    return NULL;
}

/* Shell control functions */
void toggle_kernel_shell(void) {
    shell_lock();

    if (atomic_xor(&shell_exit, true) && atomic_load(&shell_dead)) {
        int err = thread_create(NULL, kernel_shell, NULL, THREAD_CREATE_SCHED, NULL);
        if (err == 0) {
            atomic_store(&shell_dead, false);
        }
    }

    if (atomic_load(&shell_exit)) {
        thread_cancel(atomic_load(&shell_tid));
    }

    shell_unlock();
}

void init_kernel_shell(void) {
    /* Initialize history buffer */
    for (int i = 0; i < HISTORY_SIZE; i++) {
        cmd_history[i] = NULL;
    }
}