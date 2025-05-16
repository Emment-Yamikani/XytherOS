#include <bits/errno.h>
#include <core/debug.h>
#include <dev/dev.h>
#include <sync/atomic.h>
#include <sys/thread.h>
#include <mm/kalloc.h>
#include <mm/mem.h>
#include <limits.h>
#include <xytherOs_string.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 16
#define HISTORY_SIZE 10

atomic_int shell_tid = 0;
atomic_int shell_die = true;
atomic_int shell_dead = true;

/* Shell command history */
static char *cmd_history[HISTORY_SIZE];
static int history_index = 0;
static int history_count = 0;

void *kernel_shell(void *);

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
    snprintf(prompt, sizeof(prompt), "[\e[32mxytherOS:%d\e[m $] ", tid);
    tty_write(prompt);
}

/**
 * Read a line from cooked TTY input
 * (Simplified version that expects the TTY driver to handle line editing)
 */
static void get_cmd(char *buf, size_t size) {
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

static void add_to_history(const char *cmd) {
    if (history_count > 0 && xytherOs_string_eq(cmd, cmd_history[(history_index - 1) % HISTORY_SIZE])) {
        return;  /* Don't add duplicate consecutive commands */
    }
    
    if (cmd_history[history_index % HISTORY_SIZE]) {
        kfree(cmd_history[history_index % HISTORY_SIZE]);
    }
    
    cmd_history[history_index % HISTORY_SIZE] = xytherOs_strdup(cmd);
    history_index = (history_index + 1) % HISTORY_SIZE;
    if (history_count < HISTORY_SIZE) history_count++;
}

static void show_history(void) {
    char buf[128];
    int start = (history_index - history_count + HISTORY_SIZE) % HISTORY_SIZE;
    
    for (int i = 0; i < history_count; i++) {
        int idx = (start + i) % HISTORY_SIZE;
        snprintf(buf, sizeof(buf), "%d: %s\n", i+1, cmd_history[idx]);
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
            // struct mem_stats stats;
            // mm_get_stats(&stats);
            
            // char buf[256];
            // snprintf(buf, sizeof(buf),
            //     "Memory Stats:\n"
            //     "  Total: %zu KB\n"
            //     "  Used:  %zu KB\n"
            //     "  Free:  %zu KB\n",
            //     stats.total / 1024, stats.used / 1024, stats.free / 1024);
            // tty_write(buf);
        } else if (xytherOs_string_eq("threads", args[i])) {
            thread_t *thread;

            queue_lock(global_thread_queue);
            foreach_thread(global_thread_queue, thread, t_global_qnode) {
                thread_info_t *ti = &thread->t_info;
                char out[2048] = {0};

                thread_lock(thread);
                size_t len = snprintf(
                    out, sizeof out - 1,
                    " %3d | %3d | %3d | %3d | %8x | %8s | %8x\n",
                    thread_getpid(thread), ti->ti_tid, ti->ti_tgid, ti->ti_ktid,
                    ti->ti_flags, tget_state(ti->ti_state), ti->ti_exit
                );
                thread_unlock(thread);
                device_write(tty, 0, out, len);
            }
            queue_unlock(global_thread_queue);
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
    tty_write(
        "xytherOS Kernel Shell Commands:\n"
        "  show [mem|threads|devices] - Display system information\n"
        "  kill <tid>                 - Terminate a thread\n"
        "  run <name> [args...]       - Start a new thread\n"
        "  halt [thread]              - Stop execution\n"
        "  history                    - Show command history\n"
        "  clear                      - Clear the screen\n"
        "  help                       - Show this help\n"
    );
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

    char *name = args[1];
    char **argv = &args[1];
    int argc = 0;
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

/* Main command processor */
static void process_cmd(char **args) {
    if (!args[0]) return;

    add_to_history(args[0]);

    if (xytherOs_string_eq("show", args[0])) {
        cmd_show(args);
    }
    else if (xytherOs_string_eq("help", args[0])) {
        cmd_help();
    }
    else if (xytherOs_string_eq("clear", args[0])) {
        cmd_clear();
    }
    else if (xytherOs_string_eq("kill", args[0])) {
        cmd_kill(args);
    }
    else if (xytherOs_string_eq("run", args[0])) {
        cmd_run(args);
    }
    else if (xytherOs_string_eq("history", args[0])) {
        show_history();
    }
    else {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "%s: command not found\n", args[0]);
        tty_write(err_msg);
    }
}

/* Shell thread main function */
void *kernel_shell(void *) {
    char *cmd_tokens[MAX_ARGS + 1];  /* +1 for NULL terminator */
    atomic_store(&shell_tid, gettid());

    tty_open();
    tty_write("xytherOS Kernel Shell\nType 'help' for commands\n");

    while (!atomic_load(&shell_die)) {
        print_prompt();
        
        char buf[MAX_CMD_LEN] = {0};
        get_cmd(buf, sizeof buf);
        /* Tokenize and process command */
        int token_count = __xytherOs_tokenize_r(buf, " \t\n", cmd_tokens, MAX_ARGS);
        if (token_count < 0) {
            tty_write("Error: Memory allocation failed\n");
            continue;
        }
        cmd_tokens[token_count] = NULL;  /* Ensure NULL termination */
        
        if (token_count > 0) {
            process_cmd(cmd_tokens);
        }
        
        /* Free tokens */
        for (int i = 0; i < token_count; i++) {
            if (cmd_tokens[i]) kfree(cmd_tokens[i]);
        }
    }

    atomic_store(&shell_dead, true);
    return NULL;
}

/* Shell control functions */
void toggle_kernel_shell(void) {
    if (atomic_xor(&shell_die, true) && atomic_load(&shell_dead)) {
        int err = thread_create(NULL, kernel_shell, NULL, THREAD_CREATE_SCHED, NULL);
        if (err == 0) atomic_store(&shell_dead, false);
    }

    if (atomic_load(&shell_die)) {
        thread_cancel(atomic_load(&shell_tid));
    }
}

void init_kernel_shell(void) {
    /* Initialize history buffer */
    for (int i = 0; i < HISTORY_SIZE; i++) {
        cmd_history[i] = NULL;
    }
}