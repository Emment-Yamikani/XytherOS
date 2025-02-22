#pragma once

#include <stddef.h>

typedef struct list_head_t list_head_t;
struct list_head {
	list_head_t *prev;
	list_head_t *next;
};

#define LIST_HEAD_INIT(head) ((list_head_t){ .prev = NULL, .next = NULL })

#define LIST_HEAD(name)	list_head_t name = LIST_HEAD_INIT(name)

