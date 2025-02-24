#include <bits/errno.h>
#include <core/debug.h>
#include <string.h>
#include <sys/thread.h>

int thread_execve(thread_t *thread, const char *argp[], const char *envp[]) {
    int         err, argc;
    uc_stack_t  uc_stack_tmp;
    vmr_t       *ustack_vmr = NULL;
    char        **argpcpy, **envpcpy;

    if (thread == NULL) 
        return -EINVAL;

    thread_assert_locked(thread);
    mmap_assert_locked(thread->t_mmap);

    uc_stack_tmp = thread->t_arch.t_ustack;

    if ((err = mmap_argenvcpy(thread->t_mmap, argp, envp, &argpcpy, &argc, &envpcpy)))
        return err;

    if ((err = mmap_alloc_stack(thread->t_mmap, USTACK_SIZE, &ustack_vmr)))
        goto error;

    thread->t_arch.t_ustack = (uc_stack_t) {
        .ss_size    = __vmr_size(ustack_vmr),
        .ss_flags   = __vmr_vflags(ustack_vmr),
        .ss_sp      = (void *)__vmr_upper_bound(ustack_vmr)
    };

    if ((err = arch_thread_execve(&thread->t_arch, thread->t_info.ti_entry, argc, (const char **)argpcpy, (const char **)envpcpy)))
        goto error;    

    return 0;
error:
    thread->t_arch.t_ustack = uc_stack_tmp;

    if (ustack_vmr) mmap_remove(thread->t_mmap, ustack_vmr);

    todo("Implement a function to reverse mmap_argenvcpy(): Error[%s].\n", perror(err));

    return err;
}