#include <bits/errno.h>
#include <core/debug.h>
#include <string.h>
#include <sys/thread.h>

int thread_execve(thread_t *thread, char *const argv[], char *const envv[]) {
    if (thread == NULL) {
        return -EINVAL;
    }

    thread_assert_locked(thread);
    mmap_assert_locked(thread->t_mmap);

    assert_ne(thread->t_info.ti_entry, NULL, "Thread has no entry point.\n");

    int     err;
    vmr_t   *ustack_vmr;
    if ((err = mmap_alloc_stack(thread->t_mmap, USTACK_SIZE, &ustack_vmr))) {
        return err;
    }

    thread->t_arch.t_ustack = (uc_stack_t) {
        .ss_size    = __vmr_size(ustack_vmr),
        .ss_flags   = __vmr_vflags(ustack_vmr),
        .ss_sp      = (void *)__vmr_upper_bound(ustack_vmr)
    };

    long argc = 0;
    char *const *argp, *const *envp;
    if ((err = mmap_copy_arglist(thread->t_mmap, argv, envv, (int *)&argc, &argp, &envp))) {
        goto error;
    }

    arch_thread_t *t_arch = &thread->t_arch;
    thread_info_t *t_info = &thread->t_info;

    if ((err = arch_thread_init(t_arch, t_info->ti_entry,
        (void *)argc, (void *)argp, (void *)envp, NULL))) {
        goto error;
    }

    return 0;
error:
    mmap_remove(thread->t_mmap, ustack_vmr);
    return err;
}