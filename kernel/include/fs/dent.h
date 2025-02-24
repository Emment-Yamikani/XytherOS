#include <ds/list.h>
#include <fs/inode.h>
#include <sync/atomic.h>
#include <sync/spinlock.h>

#define DCACHE_MOUNTED      1
#define DCACHE_CAN_FREE     2
#define DCACHE_REFERENCED   4

typedef struct Dentry Dentry; 
struct Dentry {
    atomic_long     d_count;
    ulong           d_flags;
    char            *d_name;
    inode_t         *d_inode;
    Dentry          *d_parent;

    list_head_t     d_alias;
    list_head_t     d_siblings;
    list_head_t     d_children;
    spinlock_t      d_children_lock;

    spinlock_t      d_spinlock;
};

#define dentry_assert(d)            ({ assert(d, "Invalid dentry.\n"); })

#define dentry_lock(d)              ({ dentry_assert(d); spin_lock(&(d)->d_spinlock); })
#define dentry_unlock(d)            ({ dentry_assert(d); spin_unlock(&(d)->d_spinlock); })
#define dentry_trylock(d)           ({ dentry_assert(d); spin_trylock(&(d)->d_spinlock); })
#define dentry_islocked(d)          ({ dentry_assert(d); spin_islocked(&(d)->d_spinlock); })
#define dentry_recursive_lock(d)    ({ dentry_assert(d); spin_recursive_lock(&(d)->d_spinlock); })
#define dentry_assert_locked(d)     ({ dentry_assert(d); spin_assert_locked(&(d)->d_spinlock); })

extern int dentry_alloc(const char *name, Dentry **pdp);
extern int dentry_init(const char *name, Dentry *dentry);

extern int  dentry_open(Dentry *dentry);
extern void dentry_close(Dentry *dentry);
extern int  dentry_unbind(Dentry *dentry);
extern int  dentry_bind(Dentry *dir, Dentry *child);
extern int  dentry_lookup(Dentry *dir, const char *name, Dentry **pdp);