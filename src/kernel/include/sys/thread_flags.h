#pragma once

#include <core/types.h>

enum {
    OsThreadUser            = 1 << 0,   // Thread is a user-space thread
    OsThreadDetached        = 1 << 1,   // Thread is Detached; no join required
    OsThreadCanceled        = 1 << 2,   // Thread has been canceled
    OsThreadStopped         = 1 << 3,   // Thread Stopped externally
    OsThreadSuspended       = 1 << 4,   // Thread Suspended (scheduler/user)
    OsThreadInterrupted     = 1 << 5,   // Thread Interrupted from wait/sleep
    OsThreadKillExempt      = 1 << 6,   // Thread Cannot be killed
    OsThreadStateTransition = 1 << 7,   // Thread In scheduler transition state
    OsThreadNeedsFPUState   = 1 << 8,   // Thread Uses FPU/SSE (dirty state)
    OsThreadIsMain          = 1 << 9,   // Thread is Main thread of the process
    OsThreadNoPreempt       = 1 << 10,  // Thread Cannot be preempted
    OsThreadNoMigrate       = 1 << 11,  // Thread Must not migrate CPUs
    OsThreadHandlingSignal  = 1 << 12,  // Thread is handling a signal
    OsThreadKill            = 1 << 13,  // Thread is set to be kill
};

typedef ulong thread_flags_t; // thread runtime-flags

// Flags related to scheduling state
#define OsThreadSchedStateMask \
    (OsThreadStopped | OsThreadSuspended | OsThreadInterrupted | OsThreadStateTransition)

// Flags related to CPU/FPU handling
#define OsThreadCPUFlags \
    (OsThreadNoMigrate | OsThreadNeedsFPUState | OsThreadNoPreempt)

// Flags that affect termination logic
#define OsThreadExitControlMask \
    (OsThreadDetached | OsThreadCanceled | OsThreadKillExempt)

#define thread_set_flags(t, f) ({           \
    bool locked = thread_recursive_lock(t); \
    (t)->t_info.ti_flags |= (f);            \
    if (locked) thread_unlock(t);           \
})

#define thread_unset_flags(t, f) ({         \
    bool locked = thread_recursive_lock(t); \
    (t)->t_info.ti_flags &= ~(f);           \
    if (locked) thread_unlock(t);           \
})

#define thread_test_flags(t, f) ({                     \
    bool locked = thread_recursive_lock(t);            \
    thread_flags_t flags = (t)->t_info.ti_flags & (f); \
    if (locked) thread_unlock(t);                      \
    flags ? true : false;                              \
})

#define thread_toggle_flags(t, f) ({        \
    bool locked = thread_recursive_lock(t); \
    (t)->t_info.ti_flags ^= (f);            \
    if (locked) thread_unlock(t);           \
})

#define thread_set_user(t)                  thread_set_flags(t, OsThreadUser)
#define thread_set_canceled(t)              thread_set_flags(t, OsThreadCanceled)
#define thread_set_detached(t)              thread_set_flags(t, OsThreadDetached)
#define thread_set_stopped(t)               thread_set_flags(t, OsThreadStopped)
#define thread_set_suspended(t)             thread_set_flags(t, OsThreadSuspended)
#define thread_set_interrupted(t)           thread_set_flags(t, OsThreadInterrupted)
#define thread_set_kill(t)                  thread_set_flags(t, OsThreadKill)
#define thread_set_kill_exempt(t)           thread_set_flags(t, OsThreadKillExempt)
#define thread_set_state_trans(t)           thread_set_flags(t, OsThreadStateTransition)
#define thread_set_needs_FPU_state(t)       thread_set_flags(t, OsThreadNeedsFPUState)
#define thread_set_main(t)                  thread_set_flags(t, OsThreadIsMain)
#define thread_set_no_prempt(t)             thread_set_flags(t, OsThreadNoPreempt)
#define thread_set_no_migrate(t)            thread_set_flags(t, OsThreadNoMigrate)
#define thread_set_handling_signal(t)       thread_set_flags(t, OsThreadHandlingSignal)

#define thread_unset_user(t)                thread_unset_flags(t, OsThreadUser)
#define thread_unset_canceled(t)            thread_unset_flags(t, OsThreadCanceled)
#define thread_unset_detached(t)            thread_unset_flags(t, OsThreadDetached)
#define thread_unset_stopped(t)             thread_unset_flags(t, OsThreadStopped)
#define thread_unset_suspended(t)           thread_unset_flags(t, OsThreadSuspended)
#define thread_unset_interrupted(t)         thread_unset_flags(t, OsThreadInterrupted)
#define thread_unset_kill(t)                thread_unset_flags(t, OsThreadKill)
#define thread_unset_kill_exempt(t)         thread_unset_flags(t, OsThreadKillExempt)
#define thread_unset_state_trans(t)         thread_unset_flags(t, OsThreadStateTransition)
#define thread_unset_needs_FPU_state(t)     thread_unset_flags(t, OsThreadNeedsFPUState)
#define thread_unset_main(t)                thread_unset_flags(t, OsThreadIsMain)
#define thread_unset_no_prempt(t)           thread_unset_flags(t, OsThreadNoPreempt)
#define thread_unset_no_migrate(t)          thread_unset_flags(t, OsThreadNoMigrate)
#define thread_unset_handling_signal(t)     thread_unset_flags(t, OsThreadHandlingSignal)

#define thread_is_user(t)                   thread_test_flags(t, OsThreadUser)
#define thread_is_canceled(t)               thread_test_flags(t, OsThreadCanceled)
#define thread_is_detached(t)               thread_test_flags(t, OsThreadDetached)
#define thread_is_stopped(t)                thread_test_flags(t, OsThreadStopped)
#define thread_is_suspended(t)              thread_test_flags(t, OsThreadSuspended)
#define thread_is_interrupted(t)            thread_test_flags(t, OsThreadInterrupted)
#define thread_is_kill(t)                   thread_test_flags(t, OsThreadKill)
#define thread_is_kill_exempt(t)            thread_test_flags(t, OsThreadKillExempt)
#define thread_is_state_trans(t)            thread_test_flags(t, OsThreadStateTransition)
#define thread_needs_FPU_state(t)           thread_test_flags(t, OsThreadNeedsFPUState)
#define thread_is_main(t)                   thread_test_flags(t, OsThreadIsMain)
#define thread_is_no_prempt(t)              thread_test_flags(t, OsThreadNoPreempt)
#define thread_is_no_migrate(t)             thread_test_flags(t, OsThreadNoMigrate)
#define thread_is_handling_signal(t)        thread_test_flags(t, OsThreadHandlingSignal)

#define thread_toggle_user(t)               thread_toggle_flags(t, OsThreadUser)
#define thread_toggle_canceled(t)           thread_toggle_flags(t, OsThreadCanceled)
#define thread_toggle_detached(t)           thread_toggle_flags(t, OsThreadDetached)
#define thread_toggle_stopped(t)            thread_toggle_flags(t, OsThreadStopped)
#define thread_toggle_suspended(t)          thread_toggle_flags(t, OsThreadSuspended)
#define thread_toggle_interrupted(t)        thread_toggle_flags(t, OsThreadInterrupted)
#define thread_toggle_kill(t)               thread_toggle_flags(t, OsThreadKill)
#define thread_toggle_kill_exempt(t)        thread_toggle_flags(t, OsThreadKillExempt)
#define thread_toggle_state_trans(t)        thread_toggle_flags(t, OsThreadStateTransition)
#define thread_toggle_needs_FPU_state(t)    thread_toggle_flags(t, OsThreadNeedsFPUState)
#define thread_toggle_main(t)               thread_toggle_flags(t, OsThreadIsMain)
#define thread_toggle_no_prempt(t)          thread_toggle_flags(t, OsThreadNoPreempt)
#define thread_toggle_no_migrate(t)         thread_toggle_flags(t, OsThreadNoMigrate)
#define thread_toggle_handling_signal(t)    thread_toggle_flags(t, OsThreadHandlingSignal)




///

#define current_set_flags(f)                thread_set_flags(current, f)
#define current_unset_flags(f)              thread_unset_flags(current, f)
#define current_test_flags(f)               thread_test_flags(current, f)
#define current_toggle_flags(f)             thread_toggle_flags(current, f)

#define current_set_user()                  thread_set_user(current)
#define current_set_canceled()              thread_set_canceled(current)
#define current_set_detached()              thread_set_detached(current)
#define current_set_stopped()               thread_set_stopped(current)
#define current_set_suspended()             thread_set_suspended(current)
#define current_set_interrupted()           thread_set_interrupted(current)
#define current_set_kill()                  thread_set_kill(current)
#define current_set_kill_exempt()           thread_set_kill_exempt(current)
#define current_set_state_trans()           thread_set_state_trans(current)
#define current_set_needs_FPU_state()       thread_set_needs_FPU_state(current)
#define current_set_main()                  thread_set_main(current)
#define current_set_no_prempt()             thread_set_no_prempt(current)
#define current_set_no_migrate()            thread_set_no_migrate(current)
#define current_set_handling_signal()       thread_set_handling_signal(current)

#define current_unset_user()                thread_unset_user(current)
#define current_unset_canceled()            thread_unset_canceled(current)
#define current_unset_detached()            thread_unset_detached(current)
#define current_unset_stopped()             thread_unset_stopped(current)
#define current_unset_suspended()           thread_unset_suspended(current)
#define current_unset_interrupted()         thread_unset_interrupted(current)
#define current_unset_kill()                thread_unset_kill(current)
#define current_unset_kill_exempt()         thread_unset_kill_exempt(current)
#define current_unset_state_trans()         thread_unset_state_trans(current)
#define current_unset_needs_FPU_state()     thread_unset_needs_FPU_state(current)
#define current_unset_main()                thread_unset_main(current)
#define current_unset_no_prempt()           thread_unset_no_prempt(current)
#define current_unset_no_migrate()          thread_unset_no_migrate(current)
#define current_unset_handling_signal()     thread_unset_handling_signal(current)

#define current_is_user()                   thread_is_user(current)
#define current_is_canceled()               thread_is_canceled(current)
#define current_is_detached()               thread_is_detached(current)
#define current_is_stopped()                thread_is_stopped(current)
#define current_is_suspended()              thread_is_suspended(current)
#define current_is_interrupted()            thread_is_interrupted(current)
#define current_is_kill()                   thread_is_kill(current)
#define current_is_kill_exempt()            thread_is_kill_exempt(current)
#define current_is_state_trans()            thread_is_state_trans(current)
#define current_needs_FPU_state()           thread_needs_FPU_state(current)
#define current_is_main()                   thread_is_main(current)
#define current_is_no_prempt()              thread_is_no_prempt(current)
#define current_is_no_migrate()             thread_is_no_migrate(current)
#define current_is_handling_signal()        thread_is_handling_signal(current)

#define current_toggle_user()               thread_toggle_user(current)
#define current_toggle_canceled()           thread_toggle_canceled(current)
#define current_toggle_detached()           thread_toggle_detached(current)
#define current_toggle_stopped()            thread_toggle_stopped(current)
#define current_toggle_suspended()          thread_toggle_suspended(current)
#define current_toggle_interrupted()        thread_toggle_interrupted(current)
#define current_toggle_kill()               thread_toggle_kill(current)
#define current_toggle_kill_exempt()        thread_toggle_kill_exempt(current)
#define current_toggle_state_trans()        thread_toggle_state_trans(current)
#define current_toggle_needs_FPU_state()    thread_toggle_needs_FPU_state(current)
#define current_toggle_main()               thread_toggle_main(current)
#define current_toggle_no_prempt()          thread_toggle_no_prempt(current)
#define current_toggle_no_migrate()         thread_toggle_no_migrate(current)
#define current_toggle_handling_signal()    thread_toggle_handling_signal(current)