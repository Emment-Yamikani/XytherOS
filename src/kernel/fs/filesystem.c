#include <fs/fs.h>
#include <bits/errno.h>
#include <core/assert.h>
#include <mm/kalloc.h>
#include <string.h>
#include <sync/atomic.h>

static spinlock_t *fsIDlock = &SPINLOCK_INIT();

static u64 fsIDalloc(void) {
    static atomic_t fs_ID = 0;
    spin_lock(fsIDlock);
    atomic_t id = __atomic_add_fetch(&fs_ID, 1, __ATOMIC_SEQ_CST);
    spin_unlock(fsIDlock);
    return id;
}

int fsalloc(filesystem_t **pfs) {
    if (pfs == NULL) {
        return -EINVAL;
    }

    filesystem_t *fs = kzalloc(sizeof *fs);
    if (fs == NULL) {
        return -ENOMEM;
    }

    int err = qnode_init(&fs->fs_superblocks, (void *)fs);
    if (err) {
        kfree(fs);
        return err;
    }

    fs->fs_count= 1;
    fs->fs_id   = fsIDalloc();
    fs->fs_lock = SPINLOCK_INIT();

    fslock(fs);

    *pfs = fs;

    return 0;
}

void fs_free(filesystem_t *fs) {
    if (!fsislocked(fs)) {
        fslock(fs);
    }


    fs_put(fs);

    if (fs->fs_count <= 0) {
        if (fs->fs_name) {
            fs_unsetname(fs);
        }

        int err = qnode_drain(&fs->fs_superblocks);
        assert_eq(err, 0, "Failed to drain FS->superblock_head.\n");

        fsunlock(fs);
        kfree(fs);
        return;
    }

    fsunlock(fs);
}

int fs_create(const char *name, iops_t *iops, filesystem_t **pfs) {
    int err = 0;
    filesystem_t *fs = NULL;
    if (name == NULL || iops == NULL || pfs == NULL) {
        return -EINVAL;
    }

    if ((err = fsalloc(&fs))) {
        return err;
    }

    if ((err = fs_setname(fs, name))) {
        goto error;
    }

    if ((err = qnode_init(&fs->fs_superblocks, NULL))) {
        goto error;
    }

    if ((err = qnode_init(&fs->fslist_node, fs))) {
        goto error;
    }

    fs->fs_iops = iops;

    *pfs = fs;

    return 0;
error:
    if (fs) {
        fs_free(fs);
    }

    return err;
}

void fs_dup(filesystem_t *fs) {
    fsassert_locked(fs);
    fs->fs_count++;
}

void fs_put(filesystem_t *fs) {
    fsassert_locked(fs);
    fs->fs_count--;
}

long fs_count(filesystem_t *fs) {
    fsassert_locked(fs);
    return fs->fs_count;
}

int fs_setname(filesystem_t *fs, const char *fsname) {
    char *name = NULL;

    fsassert_locked(fs);

    if (fs == NULL || fsname == NULL) {
        return -EINVAL;
    }

    if ((name = strdup(fsname)) == NULL) {
        return -ENOMEM;
    }

    fs->fs_name = name;

    return 0;
}

void fs_unsetname(filesystem_t *fs) {
    fsassert_locked(fs);
    if (fs == NULL) {
        return;
    }

    if (fs->fs_name) {
        kfree(fs->fs_name);
    }
}

int fs_set_iops(filesystem_t *fs, iops_t *iops) {
    fsassert_locked(fs);
    if (fs == NULL || iops == NULL) {
        return -EINVAL;
    }

    fs->fs_iops = iops;
    return 0;
}

int fs_add_superblock(filesystem_t *fs, sblock_t *sb) {
    if (fs == NULL || sb == NULL) {
        return -EINVAL;
    }

    fsassert_locked(fs);

    int err = qnode_enqueue(&fs->fs_superblocks, &sb->sb_fsnode, QUEUE_UNIQUE);
    if (err) { return err; }

    sb->sb_iops = fs->fs_iops;

    return 0;
}

int fs_del_superblock(filesystem_t *fs, sblock_t *sb) {
    if (fs == NULL || sb == NULL) {
        return -EINVAL;
    }

    fsassert_locked(fs);
    return qnode_list_remove(&fs->fs_superblocks, &sb->sb_fsnode);
}