#include <core/debug.h>
#include <dev/dev.h>
#include <ds/list.h>
#include <fs/fs.h>
#include <string.h>
#include <sys/thread.h>
#include <sync/mutex.h>
#include <mm/kalloc.h>
#include <sys/schedule.h>
#include <arch/x86_64/isr.h>

CONDITION_VARIABLE(wait);

void thread(void) {
    uc_stack_t ss;

    ss.ss_flags = 0;
    ss.ss_size  = KSTACK_SIZE;
    ss.ss_sp    = kmalloc(KSTACK_SIZE) + KSTACK_SIZE;

    sigaltstack(&ss, NULL);
    cond_signal(wait);

    sigset_t set;
    sigsetempty(&set);

    sigsuspend(&set);

    debuglog();
    loop();
} BUILTIN_THREAD(thread, thread, NULL);

void signal_handler(int signo, siginfo_t *siginfo, ucontext_t *uctx) {
    printk("\n%s(siginfo: %d, uctx: %p);\n",
        signal_str[signo - 1],
        siginfo->si_signo, uctx
    );
}

__noreturn void kthread_main(void) {
    sigset_t    set;
    sigaction_t act;

    sigsetfill(&set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    memset(&act, 0, sizeof act);

    act.sa_flags   |= SA_SIGINFO;
    act.sa_handler = signal_handler;

    sigaction(SIGCANCEL, &act, NULL);

    memset(&act, 0, sizeof act);

    act.sa_handler = signal_handler;
    act.sa_flags   |= SA_SIGINFO | SA_ONSTACK;
    sigaction(SIGINT, &act, NULL);

    thread_builtin_init();

    cond_wait(wait);
    pthread_kill(2, SIGINT);
    pthread_kill(2, SIGCANCEL);
    pthread_kill(2, SIGINT);
    pthread_kill(2, SIGCANCEL);
    pthread_kill(2, SIGINT);
    pthread_kill(2, SIGCANCEL);

    debuglog();
    loop();
}
