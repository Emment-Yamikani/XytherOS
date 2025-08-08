#pragma once

#include <ds/queue.h>
#include <sync/spinlock.h>
#include <sys/_time.h>

typedef struct await_event {
    long            ev_count;      // >0: available events
    bool            ev_triggered;
    bool            ev_broadcast;
    queue_t         ev_waitqueue;  // ev_waitqueue for waiting threads
    spinlock_t      ev_lock;       // sync lock for this struct
} await_event_t;

/* Function Prototypes */

extern int  await_event_init(await_event_t *ev);
extern void await_event_destroy(await_event_t *ev);

extern int  await_event(await_event_t *ev, spinlock_t *spinlock);
extern int  await_event_timed(await_event_t *ev, const timespec_t *timeout, spinlock_t *spinlock);
extern int  try_await_event(await_event_t *ev);
extern void await_event_signal(await_event_t *ev);
extern void await_event_broadcast(await_event_t *ev);