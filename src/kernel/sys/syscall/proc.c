#include <sys/proc.h>

pid_t getppid(void) {
    pid_t pid = 0;

    proc_lock(curproc);

    if (curproc->parent == NULL) {
        proc_unlock(curproc);
        return -EINVAL;
    }

    proc_lock(curproc->parent);
    pid = curproc->parent->pid;
    proc_unlock(curproc->parent);
    
    return pid;
}