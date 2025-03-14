#pragma once

#include <dev/timer.h>
#include <ds/queue.h>
#include <sys/thread.h>
#include <sys/_time.h>

typedef int tmrid_t;
typedef void *tmr_cb_arg_t;
typedef void (*tmr_callback_t)(void *);

typedef struct tmr_t {
    tmrid_t         t_id;       // timer ID.
    tmr_cb_arg_t    t_arg;      // argument to pass to call back function.
    tmr_callback_t  t_callback; // callback function to call when timer expires.
    jiffies_t       t_interval; // interval if this is a periodic timer.
    jiffies_t       t_expiry;   // Expiry time.
    signo_t         t_signo;    // signal to send to the owner on expiration.
    thread_t        *t_owner;   // timer owner if any.
    queue_node_t    t_node;     // timer node in the queue of timers.
} tmr_t;

typedef struct {
    timespec_t      td_expiry;
    timespec_t      td_inteval;
    signo_t         td_signo;
    tmr_cb_arg_t    td_arg;
    tmr_callback_t  td_func;
} tmr_desc_t;

extern void timer_increment(void);
extern int timer_create(tmr_desc_t *timer, int *ptid);

typedef void (*ktimer_callback_t)(void *);

typedef struct ktimer_t ktimer_t;
struct ktimer_t {
    void                *arg;
    ktimer_callback_t   callback;
    jiffies_t           expiry;
    jiffies_t           period;
    timer_t             timerid;
    queue_node_t        node;
};

extern int      ktimer_create(jiffies_t expr, jiffies_t interval, ktimer_callback_t func, void *arg, timer_t *timerid);

extern void     ktimer_tick(void);
extern void     ktimer_delete();
extern void     ktimer_gettime();
extern void     ktimer_settime();