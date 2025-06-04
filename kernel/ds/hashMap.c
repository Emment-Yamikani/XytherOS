#include <bits/errno.h>
#include <crypto/sha256.h>
#include <ds/hashMap.h>
#include <mm/kalloc.h>
#include <string.h>

static inline hashKey default_hash(hashMap *map, const void *key) {
    return (hashKey)sha256_index((const char *)key, map->capacity);
}

static inline bool match_keys(const void *key0, const void *key1) {
    return string_eq((const char *)key0, (const char *)key1);
}

static inline void *default_copy_key(const void *key) {
    return strdup((const char *)key);
}

static inline void default_destroy_key(void *key) {
    kfree(key);
}

int hashMap_init(hashMap *map, hashMapCtx *ctx) {
    if (!map) {
        return -EINVAL;
    }

    map->size       = 0;
    map->capacity   = HASHMAP_SIZE;
    map->context    = ctx ? *ctx : (hashMapCtx){0};
    spinlock_init(&map->lock);
    return btree_init(&map->bucket_tree);
}

int hashMap_alloc(hashMapCtx *ctx, hashMap **ref) {
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

void hashEntry_destroy(hashMapCtx *ctx, hashEntry *entry) {
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

int hashEntry_alloc(hashMapCtx *ctx, const void *key, void *value, hashEntry **ref) {

    if (!key || !ref) {
        return -EINVAL;
    }

    hashEntry *entry = kzalloc(sizeof (hashEntry));
    if (entry == NULL) {
        return -ENOMEM;
    }

    entry->key = (ctx->copy ? ctx->copy : default_copy_key)(key);
    if (entry->key == NULL) {
        kfree(entry);
        return -ENOMEM;
    }

    entry->value = value;

    *ref = entry;

    return 0;
}

int hashMap_insert_entry(hashMap *map, hashEntry *entry) {
    hashMap_assert_locked(map);
    
    btree_lock(&map->bucket_tree);

    const usize idx = (map->context.hash ? map->context.hash : default_hash)(map, entry->key);

    hashEntry *head, *tail = NULL;
    int err = btree_search(&map->bucket_tree, idx, (void **)&head);
    if (err == 0) { // there is an item already.
        forlinked(node, head, node->next) {
            tail = node;
        }

        tail->next = entry;
    } else if (err == -ENOENT) {
        err = btree_insert(&map->bucket_tree, idx, (void *)entry);
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

    hashEntry *head;
    btree_lock(&map->bucket_tree);
    const usize idx = (map->context.hash ? map->context.hash : default_hash)(map, key);
    int err = btree_search(&map->bucket_tree, idx, (void **)&head);
    if (err == 0) {
        forlinked(entry, head, entry->next) {
            if ((map->context.compare ? map->context.compare : match_keys)(entry->key, key)) {
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

static bool hashMap_match_by_ptr(hashMap *, const hashEntry *entry, const void *arg) {
    return entry == (const hashEntry *)arg;
}

static bool hashMap_match_by_key(hashMap *map, const hashEntry *entry, const void *arg) {
    return (map->context.compare ? map->context.compare : match_keys)(entry->key, arg);
}

typedef bool (*match_t)(hashMap *, const hashEntry *, const void *arg);
static int hashMap_remove_internal(hashMap *map, btree_key_t btkey, match_t match, const void *arg) {
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

    if (!entry) return -ENOENT;

    map->size--;

    hashEntry_destroy(&map->context, entry);
    return 0;
}

int hashMap_remove_entry(hashMap *map, hashEntry *target) {
    if (!map || !target) return -EINVAL;

    btree_key_t btkey = (map->context.hash ? map->context.hash : default_hash)(map, target->key);
    return hashMap_remove_internal(map, btkey, hashMap_match_by_ptr, target);
}

int hashMap_remove(hashMap *map, const void *key) {
    if (!map || !key) return -EINVAL;

    btree_key_t btkey = (map->context.hash ? map->context.hash : default_hash)(map, key);
    return hashMap_remove_internal(map, btkey, hashMap_match_by_key, key);
}

size_t hashMap_size(hashMap *map) {
    hashMap_assert_locked(map);
    return map->size;
}

size_t hashMap_capacity(hashMap *map) {
    hashMap_assert_locked(map);
    return map->capacity;
}

static int hashMap_iter_init_internal(void *item, void *arg) {
    hashEntry *head = item; queue_t *queue = arg;
    if (!head || !queue) {
        return -EINVAL;
    }

    forlinked(entry, head, entry->next) {
        int err = enqueue(queue,(void *)entry, QUEUE_UNIQUE, NULL);
        if (err != 0) {
            queue_flush(queue);
            return err;
        }
    }

    return 0;
}

int hashMap_iter_init(hashMap *map, iter_t *iter) {
    if (!map || !iter) {
        return -EINVAL;
    }

    hashMap_assert_locked(map);

    btree_lock(&map->bucket_tree);
    int err = btree_traverse_inorder(map->bucket_tree.root,
        hashMap_iter_init_internal, &iter->queue);
    btree_unlock(&map->bucket_tree);

    return err;
}

int hashMap_into_iter(hashMap *map, btree_node_t *node, hashMap_iter_cb_t cb, void *arg) {
    if (cb == NULL) {
        return -EINVAL;
    }

    if (node != NULL) {
        btree_assert_locked(node->btree);
        hashMap_into_iter(map, node->left, cb, arg);
        hashEntry *head = node->data, *next;
        btree_node_t *right = node->right;
        forlinked(entry, head, next) {
            next = entry->next;
            int err = cb(map, entry, arg);
            if (err != 0) return err;
        }
        hashMap_into_iter(map, right, cb, arg);
    }
    return 0;
}

static inline int hashEntry_detach_and_destroy_entry(hashMap *map, hashEntry *entry, void *) {
    return hashMap_remove_entry(map, entry);
}

void hashMap_flush(hashMap *map) {
    if (map == NULL) {
        return;
    }

    hashMap_assert_locked(map);

    btree_lock(&map->bucket_tree);
    hashMap_into_iter(map, map->bucket_tree.root,    
        hashEntry_detach_and_destroy_entry, NULL);
    btree_unlock(&map->bucket_tree);
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