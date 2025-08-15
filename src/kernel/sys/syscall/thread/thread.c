#include <bits/errno.h>
#include <core/misc.h>
#include <core/debug.h>
#include <fs/cred.h>
#include <mm/kalloc.h>
#include <sys/thread.h>
#include <sys/sysproc.h>
#include <sys/sysprot.h>
#include <sys/_syscall.h>

/* Thread management syscalls */

int sys_park(void);
int sys_unpark(tid_t);

tid_t sys_gettid(void) {
    return gettid();
}

tid_t sys_thread_self(void) {
    return thread_self();
}

void sys_thread_yield(void) {
    sched_yield();
}

void sys_thread_exit(int status) {
    thread_exit(status);
}

int sys_thread_join(tid_t tid, void **retval) {
    return thread_join(tid, NULL, retval);
}

int sys_thread_create(tid_t *ptidp, void *attr, thread_entry_t entry, void *arg) {
    thread_t *thread;
    const int cflags = THREAD_CREATE_USER | THREAD_CREATE_SCHED;
    int err = thread_create(attr, entry, arg, (int)cflags, &thread);
    
    if (thread) {
        if (ptidp) *ptidp = thread_gettid(thread);
        thread_unlock(thread);
    }

    return err;
}

int sys_set_thread_area(void *addr);
int sys_get_thread_area(void *addr);
