#include <bits/errno.h>
#include <core/debug.h>
#include <string.h>
#include <sys/thread.h>

static int copy_argslist(mmap_t *mmap, char *const args[], long *pargc, char **pvec[], vmregion_node_t **pvmr) {
    if (mmap == NULL || pvec == NULL || pvmr == NULL) {
        return -EINVAL;
    }

    char **arg_list = (char **)args;

    size_t count    = 1;
    size_t buf_size = 0;

    foreach(arg, arg_list) {
        count += 1;
        buf_size += strlen(arg) + 1;
    }

    size_t region_size = ALIGN4KUP(buf_size + (count * sizeof (char *)));

    vmregion_node_t *vec_vmr_node;
    int err = mmap_alloc_range(mmap, 0, region_size, VmregionData | VmregionDontExpand, &vec_vmr_node);
    if (err) { return err; }

    vmregion_t *vec_vmr = vmregion_from_vmregion_node(vec_vmr_node);

    if ((err = vmregion_map_all(vec_vmr))) {
        mmap_remove_vmregion_node(mmap, vec_vmr_node, NULL);
        return err;
    }

    char *buffer  = (char *)__vmregion_start(vec_vmr);
    char **vector = (char **)ALIGN_UP(buffer + buf_size, 16);

    assert(&vector[count] < (char **)__vmregion_end(vec_vmr), "Bounds check.\n");

    size_t index = 0;
    foreach(arg, arg_list) {
        const size_t arglen = strlen(arg);
        safestrncpy(buffer, arg, arglen);

        vector[index++] = buffer;

        buffer += arglen;
    }

    vector[count] = NULL;

    if (pargc) { *pargc = count; }

    *pvmr = vec_vmr_node;
    *pvec = vector;

    return 0;
}

static int thread_execve_copy_args(thread_t *thread, char *const argv[], char *const envv[]) {
    if (thread == NULL) {
        return -EINVAL;
    }

    mmap_t *mmap = thread->t_mmap;

    char **argp;
    long argc = 0;
    vmregion_node_t *argp_vmregion_node;

    int err = copy_argslist(mmap, argv, &argc, &argp, &argp_vmregion_node);
    if (err) { return err; }

    char **envp;
    vmregion_node_t *envp_vmregion_node;
    if ((err = copy_argslist(mmap, envv, NULL, &envp, &envp_vmregion_node))) {
        mmap_remove_vmregion_node(mmap, argp_vmregion_node, NULL);
        return err;
    }

    arch_thread_t *tarch = &thread->t_arch;
    thread_info_t *tinfo = &thread->t_info;

    if ((err = arch_thread_init(tarch, tinfo->ti_entry, (void *)argc, argp, envp, NULL))) {
        mmap_remove_vmregion_node(mmap, argp_vmregion_node, NULL);
        mmap_remove_vmregion_node(mmap, envp_vmregion_node, NULL);
        return err;
    }

    return 0;
}

int thread_execve(thread_t *thread, char *const argv[], char *const envv[]) {
    if (thread == NULL) {
        return -EINVAL;
    }

    thread_assert_locked(thread);
    mmap_assert_locked(thread->t_mmap);

    assert_ne(thread->t_info.ti_entry, NULL, "Thread has no entry point.\n");

    int err;
    vmregion_node_t *ustack_vmregion_node;
    if ((err = mmap_alloc_stack(thread->t_mmap, USTACK_SIZE, &ustack_vmregion_node))) {
        return err;
    }

    vmregion_t *ustack_vmregion = vmregion_from_vmregion_node(ustack_vmregion_node);

    thread->t_arch.t_ustack = (uc_stack_t) {
        .ss_size    = __vmregion_size(ustack_vmregion),
        .ss_flags   = __vmregion_to_pte_flags(ustack_vmregion),
        .ss_sp      = (void *)__vmregion_upper_bound(ustack_vmregion)
    };

    if ((err = thread_execve_copy_args(thread, argv, envv))) {
        goto error;
    }

    return 0;
error:
    mmap_remove_vmregion_node(thread->t_mmap, ustack_vmregion_node, NULL);
    return err;
}