#pragma once

#include <dev/vt_key.h>

// ────────────── Modifier Flags ──────────────
#define KBD_MOD_SHIFT_L      (1 << 0)
#define KBD_MOD_SHIFT_R      (1 << 1)
#define KBD_MOD_CTRL_L       (1 << 2)
#define KBD_MOD_CTRL_R       (1 << 3)
#define KBD_MOD_ALT_L        (1 << 4)
#define KBD_MOD_ALT_R        (1 << 5)
#define KBD_MOD_CAPSLOCK     (1 << 6)
#define KBD_MOD_NUMLOCK      (1 << 7)

// ────────────── Event Flags ──────────────
typedef enum {
    EV_NONE         = (0 << 0),
    EV_MAKE         = (1 << 0),  // Key pressed
    EV_REPEAT       = (1 << 1),  // Held long enough for typematic repeat
    EV_SYNTHETIC    = (1 << 2),  // Software-generated event
    EV_EXTENDED     = (1 << 3),  // Extended scancode
} ev_flag_t;

// ────────────── Event Structure ──────────────
typedef struct {
    // TODO: Track keyboard hardware ID.
    time_t      ev_timestamp;  // Timestamp of keyboard event.
    uint8_t     ev_scode;      // Raw scancode (may be index into extended table).
    vt_key_t    ev_vt_key;     // Mapped virtual key.
    uint16_t    ev_flags;      // EV_MAKE / BREAK / REPEAT / etc.
} kbd_event_t;

// ────────────── Helper Macro ──────────────
#define KEY_CTRL(k)   ((k) - '@')  // e.g. KEY_CTRL('C') == 0x03 (SIGINT)

int kbd_get_event(kbd_event_t *ev);
int kbd_inject_event(kbd_event_t *ev);
int async_kbd_inject_event(kbd_event_t *ev);

#define EV_READ     0x100
#define EV_WRITE    0x200
#define EV_GRAB     0x400
#define EV_UNGRAB   0x800