#include <bits/errno.h>
#include <sys/thread.h>

static int do_sigaction(int signo, sigaction_t *act, sigaction_t *oact) {
    sigaction_t *p_act;

    if (SIGBAD(signo))
        return -EINVAL;

    p_act = &current->t_signals->sig_action[signo - 1];

    signal_lock(current->t_signals);

    if (oact) *oact = *p_act;
    
    if (act) {
        sigsetdelmask(&act->sa_mask, SIGMASK(SIGKILL) | SIGMASK(SIGSTOP));

        *p_act = *act;
        
        if (sig_handler_ignored(sig_handler(current, signo), signo)) {
            queue_lock(&current->t_signals->sig_queue[signo - 1]);
            sigqueue_flush(&current->t_signals->sig_queue[signo - 1]);
            queue_unlock(&current->t_signals->sig_queue[signo - 1]);

            foreach_thread(current->t_group, t_group_qnode) {
                thread_lock(thread);
                if (sigismember(&thread->t_sigpending, signo)) {
                    queue_lock(&thread->t_sigqueue[signo - 1]);
                    sigqueue_flush(&thread->t_sigqueue[signo - 1]);
                    queue_lock(&thread->t_sigqueue[signo - 1]);
                    sigsetdel(&thread->t_sigpending, signo);
                }
                thread_unlock(thread);
            }
        }
    }
    signal_unlock(current->t_signals);
    return 0;
}

int sigaction(int signo, const sigaction_t *act, sigaction_t *oact) {
    int err;
    sigaction_t new_act, old_act;

    if (SIGBAD(signo))
        return -EINVAL;
    
    if (act)
        new_act = *act;

    err = do_sigaction(signo, act ? &new_act : NULL, oact ? &old_act : NULL);

    if (err)
        return err;

    if (oact)
        *oact = old_act;
    
    return 0;
}