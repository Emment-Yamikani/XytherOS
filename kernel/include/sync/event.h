#pragma once

#include <ds/queue.h>
#include <sync/spinlock.h>
#include <sys/_time.h>

typedef struct await_event {
    long            count;      // >0: available events, <0: waiting threads
    queue_t         waitqueue;  // waitqueue for waiting threads
    spinlock_t      lock;       // sync lock for this struct
} await_event_t;

/* Function Prototypes */
extern int  await_event_init(await_event_t *ev);
extern int  await_event(await_event_t *ev);
extern int  await_event_timed(await_event_t *ev, const timespec_t *timeout);
extern int  try_await_event(await_event_t *ev);
extern void await_event_wakeup(await_event_t *ev);
extern void await_event_wakeup_all(await_event_t *ev);
extern void await_event_destroy(await_event_t *ev);