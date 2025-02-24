#pragma once

#include <ds/list.h>
#include <ds/stack.h>
#include <fs/inode.h>
#include <sync/atomic.h>
#include <sync/spinlock.h>

#define DCACHE_MOUNTED      1
#define DCACHE_CAN_FREE     2
#define DCACHE_REFERENCED   4

typedef struct dentry {
    atomic_long     d_count;
    ulong           d_flags;
    char            *d_name;
    inode_t         *d_inode;
    struct dentry   *d_parent;

    list_head_t     d_alias;
    list_head_t     d_siblings;
    list_head_t     d_children;
    spinlock_t      d_children_lock;

    stack_t         d_mnt_stack;

    spinlock_t      d_spinlock;
} dentry_t;

#define dassert(d)            ({ assert(d, "Invalid dentry.\n"); })

#define dlock(d)              ({ dassert(d); spin_lock(&(d)->d_spinlock); })
#define dunlock(d)            ({ dassert(d); spin_unlock(&(d)->d_spinlock); })
#define dtrylock(d)           ({ dassert(d); spin_trylock(&(d)->d_spinlock); })
#define dislocked(d)          ({ dassert(d); spin_islocked(&(d)->d_spinlock); })
#define drecursive_lock(d)    ({ dassert(d); spin_recursive_lock(&(d)->d_spinlock); })
#define dassert_locked(d)     ({ dassert(d); spin_assert_locked(&(d)->d_spinlock); })

extern int dalloc(const char *name, dentry_t **pdp);
extern int dinit(const char *name, dentry_t *dentry);

extern dentry_t *dget(dentry_t *dentry);
extern int  dopen(dentry_t *dentry);
extern void dclose(dentry_t *dentry);
extern int  dunbind(dentry_t *dentry);
extern int  dbind(dentry_t *dir, dentry_t *child);
extern int  dlookup(dentry_t *dir, const char *name, dentry_t **pdp);