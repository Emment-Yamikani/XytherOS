#include <bits/errno.h>
#include <sys/thread.h>

int sigaction(int signo, const sigaction_t *act, sigaction_t *oact) {
    sigset_t    mask;
    sigaction_t *p_act;

    if (SIGBAD(signo))
        return -EINVAL;

    p_act = &current->t_signals->sig_action[signo - 1];

    signal_lock(current->t_signals);

    if (oact) *oact = *p_act;
    
    if (act) {
        bool was_ignored = p_act->sa_handler == SIG_IGN;
        *p_act = *act;
        
    }
    signal_unlock(current->t_signals);
    return 0;
}