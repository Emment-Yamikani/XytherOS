#include <bits/errno.h>
#include <core/types.h>
#include <ds/list.h>

int list_empty(list_head_t *head) {
    return head->next == head;
}

void __list_add(list_head_t *node, list_head_t *prev, list_head_t *next) {
    node->prev = prev;
    prev->next = node;
    node->next = next;
    next->prev = node;
}

void list_add(list_head_t *node, list_head_t *head) {
    __list_add(node, head, head->next);
}

void list_add_tail(list_head_t *node, list_head_t *head) {
    __list_add(node, head->prev, head);
}

void __list_remove(list_head_t *prev, list_head_t *next) {
    next->prev = prev;
    prev->next = next;
}

void list_remove(list_head_t *node) {
    __list_remove(node->prev, node->next);
}

void list_remove_tail(list_head_t *head) {
    if (!list_empty(head)) {
        __list_remove(head->prev->prev, head);
    }
}

void list_replace(list_head_t *old, list_head_t *new) {
    new->next = old->next;
    new->next->prev = new;
    new->prev = old->prev;
    new->prev->next = new;
    INIT_LIST_HEAD(old);
}

void __list_rellocate_node(list_head_t *node, list_head_t *prev, list_head_t *next) {
    __list_remove(node->prev, node->next);
    __list_add(node, prev, next);
}

void list_rellocate_node(list_head_t *node, list_head_t *head) {
    __list_rellocate_node(node, head, head->next);
}

list_head_t* list_slice(list_head_t *head, list_head_t *start, list_head_t *end) {
    static list_head_t slice; // Static to ensure the list persists after the function returns
    INIT_LIST_HEAD(&slice);   // Initialize the new list

    if (start == end || list_empty(head)) {
        return &slice; // Return an empty list if the slice is invalid or the list is empty
    }

    // Disconnect the slice from the original list
    start->prev->next = end;
    end->prev = start->prev;

    // Connect the slice to the new list
    slice.next = start;
    slice.prev = end->prev;
    start->prev = &slice;
    end->prev->next = &slice;

    return &slice;
}

void list_merge(list_head_t *list1, list_head_t *list2) {
    if (list_empty(list2)) {
        return; // If list2 is empty, nothing to merge
    }

    // Connect the last node of list1 to the first node of list2
    list1->prev->next = list2->next;
    list2->next->prev = list1->prev;

    // Connect the last node of list2 to the sentinel node of list1
    list2->prev->next = list1;
    list1->prev = list2->prev;

    // Reinitialize list2 to make it empty
    INIT_LIST_HEAD(list2);
}

void list_slice_and_merge(list_head_t *src_list, list_head_t *dest_list, list_head_t *start, list_head_t *end) {
    list_head_t *slice = list_slice(src_list, start, end); // Extract the slice
    list_merge(dest_list, slice);                         // Merge the slice into dest_list
}

usize list_length(list_head_t *head) {
    usize length = 0;
    list_head_t *node;

    list_foreach(node, head)
        length++;

    return length;
}

int list_migrate_range(list_head_t *dst, list_head_t *src, usize start_pos, usize num_nodes, list_relloc_t whence) {
    if (list_empty(src) || num_nodes == 0) {
        return 0; // Nothing to migrate
    }

    // Find the starting node
    usize       pos = 0;
    list_head_t *start_node = NULL;
    list_head_t *node, *prev, *next;

    if (whence == LIST_RELLOC_HEAD) {
        // Start from the head
        list_foreach_safe(node, next, src) {
            if (pos == start_pos) {
                start_node = node;
                break;
            }
            pos++;
        }
    } else if (whence == LIST_RELLOC_TAIL) {
        // Start from the tail
        usize list_len = list_length(src);
        if (start_pos >= list_len) {
            return -EINVAL; // Invalid start position
        }
        pos = list_len - 1 - start_pos;
        list_foreach_reverse_safe(node, prev, src) {
            if (pos == start_pos) {
                start_node = node;
                break;
            }
            pos--;
        }
    } else {
        return -EINVAL; // Invalid whence parameter
    }

    if (!start_node) {
        return -EINVAL; // Invalid start position
    }

    // Find the end node
    list_head_t *end_node = start_node;
    for (usize i = 0; i < num_nodes - 1; i++) {
        if (end_node->next == src) {
            break; // Reached the end of the list
        }
        end_node = end_node->next;
    }

    // Extract the slice from src
    start_node->prev->next  = end_node->next;
    end_node->next->prev    = start_node->prev;

    // Insert the slice into dst
    start_node->prev = dst->prev;
    end_node->next   = dst;
    dst->prev->next  = start_node;
    dst->prev        = end_node;

    return 0;
}