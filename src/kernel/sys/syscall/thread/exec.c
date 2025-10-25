#include <bits/errno.h>
#include <core/debug.h>
#include <mm/kalloc.h>
#include <string.h>
#include <sys/binary_loader.h>
#include <sys/proc.h>
#include <sys/sysproc.h>
#include <sys/thread.h>

static int exec_copy_vec(char *const src_vec[], char *const *pdst_vec[], size_t *pcnt) {
    if (src_vec == NULL || pdst_vec == NULL) {
        return -EINVAL;
    }

    usize cnt = 0;
    char **dst_vec = NULL;

    foreach (arg, (char **)src_vec) {
        char **tmpv = (char **)krealloc(dst_vec, (cnt + 2) * sizeof(char *));
        if (tmpv == NULL) {
            tokens_free(dst_vec);
            return -ENOMEM;
        }

        dst_vec = tmpv;
        dst_vec[cnt] = strdup(arg);

        if (dst_vec[cnt] == NULL) {
            tokens_free(dst_vec);
            return -ENOMEM;
        }

        dst_vec[++cnt] = NULL;
    }

    if (pcnt) { *pcnt = cnt; }

    *pdst_vec = dst_vec;

    return 0;
}

static void exec_free_tmp_arglist(char *const argv[], char *const envp[]) {
    tokens_free((char **)argv);
    tokens_free((char **)envp);
}

/* Assumptions:
 * - tokens_free(char **vec) frees each strdup'd element and the vector itself; NULL-safe.
 * - mmap_set_focus(new, &old_pgdir) activates 'new' address space and returns previous pgdir in *old_pgdir.
 * - Kernel is globally mapped so switching CR3 while in kernel is safe.
 * - thread_exit() is noreturn.
 */
static int exec_copy_arglist(char *const argv[], char *const envv[], char *const *pargv[], char *const *penvp[]) {
    int     err    = 0;
    
    if (!pargv || !penvp) {
        return -EINVAL;
    }
    
    // copy argv.
    char  *const *argp;
    if ((err = exec_copy_vec(argv, &argp, NULL))) {
        return err;
    }

    // copy envp.
    char  *const *envp;
    if ((err = exec_copy_vec(envv, &envp, NULL))) {
        tokens_free((char **)argp);
        return err;
    }

    *pargv = argp;
    *penvp = envp;

    return 0;
}

static int exec_verify_image(inode_t *image_inode) {
    if (image_inode == NULL) {
        return -EINVAL;
    }

    iassert_locked(image_inode);

    int retval = 0;

    /// check file  type and only execute an appropriate file type.
    if (IISDEV(image_inode)) {
        retval = -ENOEXEC;
    } else if (IISDIR(image_inode)) {
        retval = -EISDIR;
    } else if (IISFIFO(image_inode)) {
        retval = -ENOEXEC;
    } else if (IISSYM(image_inode)) {
        retval = -ENOEXEC;
    }

    return retval;
}

static int exec_get_new_mmap(mmap_t **pmm) {
    int err;
    mmap_t *mmap;
    if ((err = mmap_alloc(&mmap))) {
        return err;
    }

    if ((err = mmap_set_focus(mmap, NULL))) {
        mmap_free(mmap);
        return err;
    }

    *pmm = mmap;
    return 0;
}

static int exec_spawn_thread(mmap_t *mmap, char *const argv[], char *const envp[], thread_t **pthread) {
    int err;

    if (!mmap) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    thread_t *thread;
    if ((err = thread_alloc(KSTACK_SIZE, THREAD_CREATE_USER, &thread))) {
        return err;
    }

    // add thread to current thread group.
    if ((err = thread_join_group(current, thread))) {
        goto error;
    }

    thread_info_t *t_info = &thread->t_info;

    t_info->ti_entry = mmap->entry;
    thread->t_mmap   = mmap;

    if ((err = thread_execve(thread, argv, envp))) {
        goto error;
    }

    if ((err = enqueue_global_thread(thread))) {
        goto error;
    }

    if ((err = thread_schedule(thread))) {
        goto error;
    }

    *pthread = thread;
    return 0;
error:
    thread_free(thread);
    return err;
}

static int exec_thread_resign(thread_t *thread) {
    if (thread == NULL) {
        return -EINVAL;
    }

    thread_set_main(thread);
    current_unset_main();

    return 0;
}

int exec_load_image(const char *path, mmap_t *mmap) {
    if (path == NULL || mmap == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    if (!arch_active_pdbr(mmap->pgdir)) {
        return -EACCES;
    }

    dentry_t *dentry;
    int err = vfs_lookup(path, NULL, O_EXEC, &dentry);
    if (err) {
        return err;
    }

    inode_t *binary = dentry->d_inode;
    if (binary == NULL) {
        dclose(dentry);
        return -EINVAL;
    }

    ilock(binary);
    if ((err = exec_verify_image(binary))) {
        iunlock(binary);
        dclose(dentry);

        return err;
    }

    // Iterate over all installed binary loaders. trying to load the file.
    foreach_binary_loader() {
        /// check the binary image to make sure it is a valid program file.
        if ((err = loader->check(binary))) {
            iunlock(binary);
            dclose(dentry);

            return err;
        }

        /// load the binary image into memory in readiness for execution.
        if ((err = loader->load(binary, mmap)) == 0) {
            iunlock(binary);
            /**
             * @brief close this dentry.
             * dentry->d_inode must have been opened.
             * To remain persistent.
             */
            dclose(dentry);
            return 0;
        }
    }

    /// binary file not loaded ???.
    iunlock(binary);
    dclose(dentry);

    return err;
}

int execve(const char *path, char *const argv[], char *const envp[]) {
    if (path == NULL) {
        return -EINVAL;
    }

    char *const *tmpargv, *const *tmpenvp;
    int err = exec_copy_arglist(argv, envp, &tmpargv, &tmpenvp);
    if (err) {
        return err;
    }

    mmap_t *mmap;
    if ((err = exec_get_new_mmap(&mmap))) {
        goto mmap_error;
    }
    
    if ((err = exec_load_image(path, mmap))) {
        goto load_error;
    }
    
    thread_t *thread;
    if ((err = exec_spawn_thread(mmap, tmpargv, tmpenvp, &thread))) {
        goto spawn_error;
    }

    thread_unlock(thread);
    
    proc_lock(curproc);
    // thread_entry_t entry = curproc->entry;
    curproc->entry = mmap->entry;
    curproc->mmap  = mmap;
    curproc->main_thread = thread;
    
    proc_unlock(curproc);
    
    mmap_unlock(mmap);

    exec_thread_resign(thread);

    exec_free_tmp_arglist(tmpargv, tmpenvp);

    current_lock();
    mmap_free(current->t_mmap);
    current->t_mmap = mmap;
    current_unlock();

    thread_exit(0);

    __builtin_unreachable(); // This code SHOULD NEVER be reached.
spawn_error:
load_error:
    mmap_free(mmap);

mmap_error:
    exec_free_tmp_arglist(tmpargv, tmpenvp);
    return err;
}