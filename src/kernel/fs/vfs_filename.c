#include <bits/errno.h>
#include <core/assert.h>
#include <core/defs.h>
#include <fs/filename.h>
#include <lib/printk.h>
#include <mm/kalloc.h>
#include <string.h>

__unused static void vfs_ref(vfs_filename_t *fn) {
    if (fn) {
        atomic_fetch_add(&fn->refcnt, 1);
    }
}

static void vfs_unref(vfs_filename_t *fn) {
    if (!fn) return;

    if (atomic_fetch_sub(&fn->refcnt, 1) == 1) {
        kfree(fn->name);
        kfree(fn);
    }
}

int vfs_create_filename(const char *name, vfs_filename_t **rfname) {
    if (!name || !rfname) return -EINVAL;

    size_t len = strlen(name);
    vfs_filename_t *fn = kzalloc(sizeof(vfs_filename_t));
    if (!fn) return -ENOMEM;

    fn->name = kzalloc(len + 1);
    if (!fn->name) {
        kfree(fn);
        return -ENOMEM;
    }

    memcpy(fn->name, name, len + 1);
    fn->length = len;

    atomic_init(&fn->refcnt, 1);

    *rfname = fn;
    return 0;
}

void vfs_destroy_filename(vfs_filename_t *fn) {
    vfs_unref(fn);
}

bool vfs_compare_filenames(const vfs_filename_t *a, const vfs_filename_t *b) {
    if (!a || !b) return false;
    if (a->length != b->length) return false;
    if (a == b) return true;
    return memcmp(a->name, b->name, a->length) == 0;
}

bool vfs_compare_raw_filename(const vfs_filename_t *fn, const char *raw) {
    if (!fn || !raw) return false;
    size_t raw_len = strlen(raw);
    if (raw_len != fn->length) return false;
    if (fn->name == raw) return true;
    return memcmp(fn->name, raw, raw_len) == 0;
}

int vfs_rename_filename(vfs_filename_t *fn, const char *new_name) {
    if (!fn || !new_name) return -EINVAL;

    size_t new_len = strlen(new_name);
    char *new_buf = kzalloc(new_len + 1);
    if (!new_buf) return -ENOMEM;

    memcpy(new_buf, new_name, new_len + 1);

    kfree(fn->name);
    fn->name = new_buf;
    fn->length = new_len;

    return 0;
}

int vfs_clone_filename(const vfs_filename_t *src, vfs_filename_t **rdst) {
    if (!src || !rdst) return -EINVAL;

    vfs_filename_t *dup = kzalloc(sizeof(vfs_filename_t));
    if (!dup) return -ENOMEM;

    dup->name = kzalloc(src->length + 1);
    if (!dup->name) {
        kfree(dup);
        return -ENOMEM;
    }

    memcpy(dup->name, src->name, src->length + 1);
    dup->length = src->length;
    atomic_init(&dup->refcnt, 1);

    *rdst = dup;
    return 0;
}

void vfs_dump_filename(const vfs_filename_t *fn) {
    if (fn == NULL) {
        return;
    }

    printk("vfs_filename: \e[32m%s\e[0m, length: %d, refcnt: %d\n",
        fn->name, fn->length, fn->refcnt);
}

size_t vfs_filename_length(const vfs_filename_t *fn) {
    assert(fn, "Invalid \e[35mvfs_filename_t *\e[0m pointer.\n");
    return fn->length;
}