#include <ds/queue.h>
#include <fs/inode.h>
#include <sync/atomic.h>
#include <sync/spinlock.h>

typedef struct Dentry Dentry; 
struct Dentry {
    atomic_long     d_count;
    atomic_ulong    d_flags;
    char            *d_name;
    inode_t         *d_inode;

    Dentry          *d_parent;
    Dentry          *d_children;

    queue_node_t    d_alias;
    queue_node_t    d_siblings;

    spinlock_t      d_spinlock;
};

#define dentry_assert(d)        ({ assert(d, "Invalid dentry.\n"); })

#define dentry_lock(d)          ({ dentry_assert(d); spin_lock(&(d)->d_spinlock); })
#define dentry_unlock(d)        ({ dentry_assert(d); spin_unlock(&(d)->d_spinlock); })
#define dentry_trylock(d)       ({ dentry_assert(d); spin_trylock(&(d)->d_spinlock); })
#define dentry_islocked(d)      ({ dentry_assert(d); spin_islocked(&(d)->d_spinlock); })
#define dentry_recursive(d)     ({ dentry_assert(d); spin_recursive(&(d)->d_spinlock); })
#define dentry_assert_locked(d) ({ dentry_assert(d); spin_assert_locked(&(d)->d_spinlock); })

extern int dentry_init(Dentry *dentry);

extern void dentry_destroy(Dentry *dentry);

extern int dentry_alloc(const char *name, Dentry **pdp);
