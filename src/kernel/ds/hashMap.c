#include <bits/errno.h>
#include <crypto/sha256.h>
#include <ds/hashMap.h>
#include <mm/kalloc.h>
#include <string.h>

static inline hashKey default_hash(hashMap *map, const void *key) {
    return (hashKey)sha256_index((const char *)key, map->capacity);
}

static inline bool default_match_keys(const void *key0, const void *key1) {
    return string_eq((const char *)key0, (const char *)key1);
}

static inline int default_copy_key(const void *key, void **ref) {
    if (!key || !ref) {
        return -EINVAL;
    }

    void *copy = strdup((const char *)key);
    if (copy == NULL) {
        return -ENOMEM;
    }

    *ref = copy;
    return 0;
}

static inline void default_destroy_key(void *key) {
    kfree(key);
}

int hashMap_init(hashMap *map, hashMapContext *ctx) {
    if (!map) {
        return -EINVAL;
    }

    map->size       = 0;
    map->capacity   = HASHMAP_SIZE;
    map->context    = ctx ? *ctx : (hashMapContext){0};
    spinlock_init(&map->lock);
    return btree_init(&map->bucket_tree);
}

int hashMap_alloc(hashMapContext *ctx, hashMap **ref) {
    if (!ref) {
        return -EINVAL;
    }

    hashMap *map = kzalloc(sizeof (hashMap));
    if (map == NULL) {
        return -ENOMEM;
    }

    int err = hashMap_init(map, ctx);
    if (err) {
        kfree(map);
        return err;
    }

    *ref = map;
    return 0;
}

void hashEntry_destroy(hashMapContext *ctx, hashEntry *entry) {
    if (entry == NULL) {
        return;
    }

    if (entry->key) {
        (ctx->destroy ? ctx->destroy : default_destroy_key)(entry->key);
        entry->key = NULL;
    }
    
    entry->value = NULL;
    kfree(entry);
}

int hashEntry_alloc(hashMapContext *ctx, const void *key, void *value, hashEntry **ref) {

    if (!key || !ref) {
        return -EINVAL;
    }

    hashEntry *entry = kzalloc(sizeof (hashEntry));
    if (entry == NULL) {
        return -ENOMEM;
    }

    int err = HASHCTX_GET_FUNC(ctx, copy, default_copy_key)(key, &entry->key);
    if (err != 0) {
        kfree(entry);
        return err;
    }

    entry->value = value;

    *ref = entry;

    return 0;
}

int hashMap_duplicate_entry(hashMapContext *ctx, hashEntry *entry, hashEntry **ref) {
    if (!entry || !ref) {
        return -EINVAL;
    }

    hashEntry *new_entry = kzalloc(sizeof *new_entry);
    if (new_entry == NULL) {
        return -ENOMEM;
    }

    int err = HASHCTX_GET_FUNC(ctx, copy, default_copy_key)(entry->key, &new_entry->key);
    if (err != 0) {
        kfree(new_entry);
        return err;
    }

    new_entry->value = entry->value;
    *ref = new_entry;
    return 0;
}

int hashMap_insert_entry(hashMap *map, hashEntry *entry) {
    hashMap_assert_locked(map);
    
    btree_lock(&map->bucket_tree);

    const hashKey hash_key = HASHCTX_GET_FUNC(&map->context, hash, default_hash)(map, entry->key);

    hashEntry *head, *tail = NULL;
    int err = btree_search(&map->bucket_tree, hash_key, (void **)&head);
    if (err == 0) { // there is an item already.
        forlinked(node, head, node->next) {
            tail = node;
        }

        tail->next = entry;
    } else if (err == -ENOENT) {
        err = btree_insert(&map->bucket_tree, hash_key, (void *)entry);
    }

    btree_unlock(&map->bucket_tree);

    if (err == 0) {
        map->size++;
    }
    return err;
}

int hashMap_insert(hashMap *map, const void *key, void *value) {
    if (!map || !key) {
        return -EINVAL;
    }

    hashMap_assert_locked(map);

    hashEntry *entry;
    int err = hashEntry_alloc(&map->context, key, value, &entry);
    if (err != 0) {
        return err;
    }

    err = hashMap_insert_entry(map, entry);
    if (err != 0) {
        hashEntry_destroy(&map->context, entry);
    }

    return err;
}

int hashMap_lookup_entry(hashMap *map, const void *key, hashEntry **ref) {
    if (!map || !key) {
        return -EINVAL;
    }

    hashMap_assert_locked(map);

    btree_lock(&map->bucket_tree);
    
    const hashKey hash_key = HASHCTX_GET_FUNC(&map->context, hash, default_hash)(map, key);
    
    hashEntry *head;
    int err = btree_search(&map->bucket_tree, hash_key, (void **)&head);
    if (err == 0) {
        forlinked(entry, head, entry->next) {
            if ((map->context.compare ? map->context.compare : default_match_keys)(entry->key, key)) {
                if (ref != NULL) {
                    *ref = entry;
                }
                btree_unlock(&map->bucket_tree);
                return 0;
            }
        }
    }
    btree_unlock(&map->bucket_tree);

    return -ENOENT;
}

int hashMap_lookup(hashMap *map, const void *key, void **ref) {
    if (!map || !key) {
        return -EINVAL;
    }

    hashMap_assert_locked(map);

    hashEntry *entry;
    int err = hashMap_lookup_entry(map, key, &entry);
    if (err == 0) {
        if (ref != NULL) {
            *ref = entry->value;
        }
        return 0;
    }

    return -ENOENT;
}

int hashMap_migrate_entry(hashMap *dst_map, hashMap *src_map, void *key) {
    if (!dst_map || !src_map || !key) {
        return -EINVAL;
    }

    hashMap_assert_locked(dst_map);
    hashMap_assert_locked(src_map);

    // Lookup the entry in the source map
    hashEntry *entry;
    int err = hashMap_lookup_entry(src_map, key, &entry);
    if (err != 0) {
        return err;
    }

    // Remove the entry from the source map
    err = hashMap_detach_entry(src_map, entry);
    if (err != 0) {
        return err;
    }

    // Re-insert the entry into the destination map
    err = hashMap_insert_entry(dst_map, entry);
    if (err != 0) {
        // If re-insertion fails, put it back in the source map
        hashMap_insert_entry(src_map, entry);
        return err;
    }

    return 0;
}

int hashMap_update(hashMap *map, const void *key, void *new_value) {
    if (!map || !key || !new_value) {
        return -EINVAL;
    }

    hashMap_assert_locked(map);

    hashEntry *entry;
    int err = hashMap_lookup_entry(map, key, &entry);
    if (err != 0) {
        return err;
    }

    entry->value = new_value;
    return 0;
}

/**
 * @brief Clones a single hash entry and inserts it into a destination hash map.
 *
 * @param src_map Source hash map (must be locked).
 * @param entry   The hash entry to clone.
 * @param dst_map Destination hash map (must be locked).
 * @return 0 on success, or a negative error code.
 */
static int hashMap_clone_iter(hashMap *src_map, hashEntry *entry, void *dst_map) {
    if (!src_map || !entry || !dst_map) {
        return -EINVAL;
    }

    hashEntry *new_entry;
    int err = hashMap_duplicate_entry(&src_map->context, entry, &new_entry);
    if (err != 0) {
        return err;
    }

    err = hashMap_insert_entry((hashMap *)dst_map, new_entry);
    if (err != 0) {
        hashEntry_destroy(&((hashMap *)dst_map)->context, new_entry);
        return err;
    }

    return 0;
}

int hashMap_clone(hashMap *src_map, hashMap **ref) {
    if (!src_map || !ref) {
        return -EINVAL;
    }

    hashMap_assert_locked(src_map);

    hashMap *dst_map;
    int err = hashMap_alloc(&src_map->context, &dst_map);
    if (err != 0) {
        return err;
    }

    hashMap_lock(dst_map);

    dst_map->capacity = src_map->capacity;

    err = hashMap_into_iter(src_map, hashMap_clone_iter, dst_map);
    if (err != 0) {
        hashMap_free(dst_map);
        return err;
    }

    hashMap_unlock(dst_map);
    *ref = dst_map;
    return 0;
}

static bool hashMap_match_by_ptr(hashMap *, const hashEntry *entry, const void *arg) {
    return entry == (const hashEntry *)arg;
}

static bool hashMap_match_by_key(hashMap *map, const hashEntry *entry, const void *arg) {
    return HASHCTX_GET_FUNC(&map->context, compare, default_match_keys)(entry->key, arg);
}

typedef bool (*hashMap_match_entry_fn_t)(hashMap *, const hashEntry *, const void *arg);
static int hashMap_remove_internal_entry(hashMap *map, btree_key_t btkey, hashMap_match_entry_fn_t match, const void *arg, bool destory_entry) {
    if (!map || !match) {
        return -EINVAL;
    }

    hashMap_assert_locked(map);

    bool locked = btree_recursive_lock(&map->bucket_tree);

    btree_node_t *btnode = btree_lookup(&map->bucket_tree, btkey);
    if (!btnode) {
        if (locked) {
            btree_unlock(&map->bucket_tree);
        }
        return -ENOENT;
    }

    hashEntry *entry= NULL;
    hashEntry *head = btnode->data;

    if (match(map, head, arg)) {
        if (head->next == NULL) {
            btree_delete_node(&map->bucket_tree, btnode);
        } else {
            btnode->data = head->next;
        }
        entry = head;
    } else {
        hashEntry *prev = head;
        forlinked(node, head->next, (prev = node)->next) {
            if (match(map, node, arg)) {
                entry = node;
                prev->next = node->next;
                break;
            }
        }
    }

    if (locked) {
        btree_unlock(&map->bucket_tree);
    }

    if (!entry) {
        return -ENOENT;
    }

    map->size--;

    if (destory_entry) {
        hashEntry_destroy(&map->context, entry);
    }

    return 0;
}

int hashMap_detach_entry(hashMap *map, hashEntry *target) {
    if (!map || !target) {
        return -EINVAL;
    }

    const hashKey hash_key = HASHCTX_GET_FUNC(&map->context, hash, default_hash)(map, target->key);
    return hashMap_remove_internal_entry(map, hash_key, hashMap_match_by_ptr, target, false);
}

int hashMap_remove_entry(hashMap *map, hashEntry *entry) {
    if (!map || !entry) {
        return -EINVAL;
    }

    const hashKey hash_key = HASHCTX_GET_FUNC(&map->context, hash, default_hash)(map, entry->key);
    return hashMap_remove_internal_entry(map, hash_key, hashMap_match_by_ptr, entry, true);
}

int hashMap_remove(hashMap *map, const void *key) {
    if (!map || !key) {
        return -EINVAL;
    }

    const hashKey hash_key = HASHCTX_GET_FUNC(&map->context, hash, default_hash)(map, key);
    return hashMap_remove_internal_entry(map, hash_key, hashMap_match_by_key, key, true);
}

size_t hashMap_size(hashMap *map) {
    hashMap_assert_locked(map);
    return map->size;
}

size_t hashMap_capacity(hashMap *map) {
    hashMap_assert_locked(map);
    return map->capacity;
}

static int hashMap_iter_common_internal(void *item, void *arg, bool use_values) {
    hashEntry   *head   = item;
    queue_t     *queue  = arg;

    if (!head || !queue) {
        return -EINVAL;
    }

    forlinked(entry, head, entry->next) {
        void *data = use_values ? entry->value : (void *)entry;
        int err = enqueue(queue, data, QUEUE_UNIQUE, NULL);
        if (err != 0) {
            queue_flush(queue);
            return err;
        }
    }

    return 0;
}

static int hashMap_iter_value_init_internal(void *item, void *arg) {
    return hashMap_iter_common_internal(item, arg, true);
}

static int hashMap_iter_init_internal(void *item, void *arg) {
    return hashMap_iter_common_internal(item, arg, false);
}

static int hashMap_iter_init_impl(hashMap *map, iter_t *iter, bool use_values) {
    if (!map || !iter) {
        return -EINVAL;
    }

    hashMap_assert_locked(map);

    btree_lock(&map->bucket_tree);
    int (*iter_cb)(void *, void *) = use_values ? hashMap_iter_value_init_internal : hashMap_iter_init_internal;
    int err = btree_traverse_inorder(map->bucket_tree.root, iter_cb, &iter->queue);
    btree_unlock(&map->bucket_tree);

    return err;
}

int hashMap_iter_init(hashMap *map, iter_t *iter) {
    return hashMap_iter_init_impl(map, iter, false); // false → enqueue entry
}

int hashMap_iter_value_init(hashMap *map, iter_t *iter) {
    return hashMap_iter_init_impl(map, iter, true); // true → enqueue entry->value
}

static int hashMap_into_iter_internal(hashMap *map, void *target, hashMap_iter_cb_t cb, void *arg) {
    if (cb == NULL) {
        return -EINVAL;
    }

    btree_node_t *node = (btree_node_t *)target;
    if (node != NULL) {
        btree_assert_locked(node->btree);
        hashMap_into_iter_internal(map, node->left, cb, arg);
        hashEntry *head = node->data, *next;
        btree_node_t *right = node->right;
        forlinked(entry, head, next) {
            next    = entry->next;
            int err = cb(map, entry, arg);
            if (err != 0) {
                return err;
            }
        }
        hashMap_into_iter_internal(map, right, cb, arg);
    }

    return 0;
}

int hashMap_into_iter(hashMap *map, hashMap_iter_cb_t cb, void *arg) {
    if (!map || !cb) {
        return -EINVAL;
    }

    hashMap_assert_locked(map);

    btree_lock(&map->bucket_tree);
    btree_node_t *root = map->bucket_tree.root;
    int err = hashMap_into_iter_internal(map, root, cb, arg);
    btree_unlock(&map->bucket_tree);

    return err;
}

static inline int hashEntry_detach_and_destroy_entry(hashMap *map, hashEntry *entry, void *) {
    return hashMap_remove_entry(map, entry);
}

void hashMap_flush(hashMap *map) {
    if (map == NULL) {
        return;
    }

    hashMap_assert_locked(map);

    hashMap_into_iter(map, hashEntry_detach_and_destroy_entry, NULL);
}

void hashMap_free(hashMap *map) {
    if (map == NULL) {
        return;
    }

    hashMap_recursive_lock(map);

    hashMap_flush(map);
    hashMap_unlock(map);

    kfree(map);
}

static int hashEntry_dump_raw(hashMap *, hashEntry *entry, void *) {
    if (entry == NULL) {
        return -EINVAL;
    }

    printk("[key: %p: value: %p]\n", entry->key, entry->value);

    return 0;
}

void hashMap_dump(hashMap *map) {
    hashMap_assert_locked(map);

    hashMap_iter_cb_t dumper = map->context.dump ? map->context.dump : hashEntry_dump_raw;
    hashMap_into_iter(map, dumper, NULL);
}