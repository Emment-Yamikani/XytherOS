#pragma once

#include <core/defs.h>
#include <core/types.h>
#include <ds/btree.h>

#define HASHMAP_SIZE 1024 /**< Default number of buckets in the hash map. */

typedef size_t hashKey;
typedef struct hashMap hashMap;
/**
 * @brief Function type for computing a hash key from a given input key.
 *
 * @param map Pointer to the hash map.
 * @param key Pointer to the input key.
 * @return Computed hash key of type hashKey.
 */
typedef hashKey (*hashMap_hash_fn_t)(hashMap *map, const void *key);

/**
 * @brief Function type for comparing two keys for equality.
 *
 * @param key1 Pointer to the first key.
 * @param key2 Pointer to the second key.
 * @return true if keys are equal, false otherwise.
 */
typedef bool (*hashMap_key_eq_fn_t)(const void *key1, const void *key2);

/**
 * @brief Function type for making a deep copy of a key.
 *
 * This function creates a heap-allocated copy of the provided key.
 * The caller is responsible for freeing the returned key.
 *
 * @param key Pointer to the original key.
 * @param ref Output pointer to receive the copied key.
 * @return 0 on success, negative error code on failure.
 */
typedef int (*hashMap_key_copy_fn_t)(const void *key, void **ref);

/**
 * @brief Function type for destroying a key and freeing its memory.
 *
 * @param key Pointer to the key to destroy.
 */
typedef void (*hashMap_key_destroy_fn_t)(void *key);

/**
 * @struct hashEntry
 * @brief Represents a single key-value pair in a hash map.
 */
typedef struct hashEntry hashEntry;
struct hashEntry {
    void        *key;   /**< Key string (copied internally). */
    hashEntry   *next;  /**< Pointer to next entry in collision chain. */
    void        *value; /**< Associated value. */
};

/**
 * @brief Callback type for iterating over hash map entries.
 * @param map The hash map.
 * @param entry The current hash entry.
 * @param arg User-defined argument.
 * @return 0 to continue iteration, non-zero to stop.
 */
typedef int (*hashMap_iter_cb_t)(hashMap *, hashEntry *, void *arg);

/**
 * @brief Context object for configuring custom key behavior in a hash map.
 */
typedef struct hashMapContext {
    /**
     * @brief Function to destroy a key (called on removal or cleanup).
     */
    hashMap_key_destroy_fn_t destroy;

    /**
     * @brief Function to copy a key (used during insertion).
     */
    hashMap_key_copy_fn_t    copy;

    /**
     * @brief Function to compute the hash of a key.
     */
    hashMap_hash_fn_t        hash;

    /**
     * @brief Function to compare two keys for equality.
     */
    hashMap_key_eq_fn_t      compare;

    hashMap_iter_cb_t        dump;
} hashMapContext;

#define HASHMAPCTX_INIT()   ((hashMapContext){0})

#define HASHMAPCTX(name)   hashMapContext *name = &HASHMAPCTX_INIT()


/**
 * @brief Safely retrieves a function pointer from a hash map context structure,
 *        falling back to a default if the context or function is missing.
 *
 * @param __ctx The pointer to the hashMapContext structure (may be NULL).
 * @param __func The name of the function field in the context struct.
 * @param __fallback The fallback function to use if the context or field is NULL.
 *
 * @return A valid function pointer, either from the context or the fallback.
 *
 * @note This macro protects against null context pointers and missing function fields.
 */
#define HASHCTX_GET_FUNC(__ctx, __func, __fallback) \
    ((__ctx) && (__ctx)->__func ? (__ctx)->__func : (__fallback))

/**
 * @struct hashMap
 * @brief Thread-safe hash map using a btree for bucket management.
 */
typedef struct hashMap {
    size_t      size;       /**< Number of entries currently in the map. */
    size_t      capacity;   /**< Maximum number of buckets available. */
    btree_t     bucket_tree;/**< Binary search tree mapping hash indices to entries. */
    hashMapContext  context;/**< HashMap key context. */
    spinlock_t  lock;       /**< Lock to ensure thread-safe operations. */
} hashMap;

#define HASHMAP(name) hashMap *name = &(hashMap) { \
    .size           = 0,                           \
    .capacity       = HASHMAP_SIZE,                \
    .bucket_tree    = BTREE_INIT(),                \
    .lock           = SPINLOCK_INIT(),             \
    .context        = HASHMAPCTX_INIT()            \
}

/** @brief Asserts the validity of the hash map pointer. */
#define hashMap_assert(map)         ({ assert(map, "Invalid HashMap.\n"); })

/** @brief Acquires the spinlock protecting the hash map. */
#define hashMap_lock(map)           ({ hashMap_assert(map); spin_lock(&(map)->lock); })

/** @brief Releases the spinlock protecting the hash map. */
#define hashMap_unlock(map)         ({ hashMap_assert(map); spin_unlock(&(map)->lock); })

/** @brief Attempts to acquire the lock without blocking. */
#define hashMap_try_lock(map)       ({ hashMap_assert(map); spin_try_lock(&(map)->lock); })

/** @brief Acquires the lock recursively (same thread can re-lock). */
#define hashMap_recursive_lock(map) ({ hashMap_assert(map); spin_recursive_lock(&(map)->lock); })

/** @brief Asserts that the current thread holds the lock. */
#define hashMap_assert_locked(map)  ({ hashMap_assert(map); spin_assert_locked(&(map)->lock); })

/**
 * @brief Frees a hash entry, including its key.
 * @param ctx A user-defined HashMap's key context.
 * @param entry Entry to destroy.
 */
extern void hashEntry_destroy(hashMapContext *ctx, hashEntry *entry);

/**
 * @brief Allocates a new hash entry with a copied key and value.
 * @param ctx A user-defined HashMap's key context.
 * @param key The key string (copied internally).
 * @param value Pointer to the value.
 * @param ref Pointer to store the allocated hash entry.
 * @return 0 on success, negative on failure.
 */
extern int hashEntry_alloc(hashMapContext *ctx, const void *key, void *value, hashEntry **ref);

/**
 * @brief Creates a duplicate of a given hash entry using the map's key copy function.
 *
 * The duplicated entry shares the same value pointer as the original but has an independent key copy.
 *
 * @param map The hash map whose context will be used for key copying.
 * @param entry The hash entry to duplicate.
 * @param ref Output pointer to store the duplicated entry.
 * @return 0 on success, negative error code on failure.
 */
extern int hashMap_duplicate_entry(hashMapContext *ctx, hashEntry *entry, hashEntry **ref);

/**
 * @brief Moves an entry from one hash map to another by key.
 *
 * This function attempts to look up an entry by `key` in the `src` hash map,
 * removes it from the source, and inserts it into the destination map (`dst`),
 * preserving its key and value. It assumes both maps are locked before the call
 * and that they use compatible `hashMapContext` logic.
 *
 * @param dst Destination hash map.
 * @param src Source hash map.
 * @param key The key of the entry to move.
 * @return 0 on success, -ENOENT if the key is not found, or another negative error code.
 */
extern int hashMap_migrate_entry(hashMap *dst, hashMap *src, void *key);

/**
 * @brief Deeply clones a hash map and all of its entries.
 *
 * @param src_map The source hash map to clone.
 * @param ref Output pointer that will receive the newly cloned hash map.
 * @return 0 on success, or a negative error code.
 */
extern int hashMap_clone(hashMap *src_map, hashMap **ref);

/**
 * @brief Updates the value of an existing key in the hash map.
 *
 * This function looks up the entry by key and replaces its value with `new_value`.
 * The key must already exist; no insertion is performed.
 *
 * @param map        Pointer to the hash map.
 * @param key        The key to search for.
 * @param new_value  The new value to assign to the key.
 * @return 0 on success,
 *         -EINVAL if any argument is invalid,
 *         -ENOENT if the key is not found.
 */
extern int hashMap_update(hashMap *map, const void *key, void *new_value);

/**
 * @brief Initializes a pre-allocated hash map structure.
 * @param map The map to initialize.
 * @param ctx A user-defined HashMap's key context.
 * @return 0 on success, negative on failure.
 */
extern int hashMap_init(hashMap *map, hashMapContext *ctx);

/**
 * @brief Allocates and initializes a new hash map.
 * @param ctx A user-defined HashMap's key context.
 * @param ref Pointer to store the new map.
 * @return 0 on success, negative on failure.
 */
extern int hashMap_alloc(hashMapContext *ctx, hashMap **ref);

/**
 * @brief Frees all memory associated with the hash map.
 * @param map The map to free.
 */
extern void hashMap_free(hashMap *map);

/**
 * @brief Removes all entries from the map without freeing the structure itself.
 * @param map The map to flush.
 */
extern void hashMap_flush(hashMap *map);

/**
 * @brief Removes an entry by key from the map.
 * @param map The hash map.
 * @param key The key of the entry to remove.
 * @return 0 on success, -ENOENT if not found.
 */
extern int hashMap_remove(hashMap *map, const void *key);

/**
 * @brief Removes a specific hash entry from the map without freeing it's memory.
 * @param map The hash map.
 * @param target The entry to remove.
 * @return 0 on success, -ENOENT if not found.
 */
extern int hashMap_remove_entry(hashMap *map, hashEntry *target);

/**
 * @brief Looks up a value by key in the map.
 * @param map The hash map.
 * @param key The key to look up.
 * @param ref Output pointer to store the value.
 * @return 0 on success, -ENOENT if not found.
 */
extern int hashMap_lookup(hashMap *map, const void *key, void **ref);

/**
 * @brief Inserts or updates a key-value pair in the map.
 * @param map The hash map.
 * @param key The key string (copied internally).
 * @param value The value pointer.
 * @return 0 on success, negative on failure.
 */
extern int hashMap_insert(hashMap *map, const void *key, void *value);

/**
 * @brief Returns the current number of entries in the map.
 * @param map The hash map.
 * @return Number of entries.
 */
extern size_t hashMap_size(hashMap *map);

/**
 * @brief Returns the total capacity (number of buckets).
 * @param map The hash map.
 * @return Total bucket capacity.
 */
extern size_t hashMap_capacity(hashMap *map);

#include <ds/iter.h>

/**
 * @brief Initializes an iterator of entries for traversing the hash map.
 * @param map The hash map.
 * @param iter Pointer to the iterator.
 * @return 0 on success, negative on failure.
 */
extern int hashMap_iter_init(hashMap *map, iter_t *iter);

/**
 * @brief Initializes an iterator of values for traversing the hash map.
 * @param map The hash map.
 * @param iter Pointer to the iterator.
 * @return 0 on success, negative on failure.
 */
extern int hashMap_iter_value_init(hashMap *map, iter_t *iter);

/**
 * @brief Iterates over entries in a specific tree node, applying a callback.
 * @param map The hash map.
 * @param cb Callback function for each entry.
 * @param arg User-defined argument for the callback.
 * @return 0 on full success, or callback return value on early exit.
 */
extern int hashMap_into_iter(hashMap *map, hashMap_iter_cb_t cb, void *arg);

/**
 * @brief Macro to iterate over all hash entries in a map using an iterator.
 * @param entry Variable holding the current hash entry.
 * @param iter Iterator variable.
 * @param map The hash map to iterate.
 */
#define hashMap_foreach_entry(map , entry, iter) \
    iter_foreach_item(entry, iter, hashMap_iter_init, map)

#define hashMap_foreach_entry_reverse(map, entry, iter) \
    iter_foreach_item_reverse(entry, iter, hashMap_iter_init, map)

#define hashMap_foreach_item(map, item, iter) \
    iter_foreach_item(item, iter, hashMap_iter_value_init, map)

#define hashMap_foreach_item_reverse(map, item, iter) \
    iter_foreach_item_reverse(item, iter, hashMap_iter_value_init, map)

/**
 * @brief Function to dump the entries of a map for viewing.
 * @param map HashMap to be dumped.
 */
extern void hashMap_dump(hashMap *map);

extern size_t hashMap_size(hashMap *map);

extern size_t hashMap_capacity(hashMap *map);