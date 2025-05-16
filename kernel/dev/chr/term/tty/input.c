#include <core/types.h>
#include <dev/tty.h>
#include <dev/kbd_event.h> 
#include <sync/spinlock.h>
#include <sys/schedule.h>
#include <sys/thread.h>

extern void toggle_kernel_shell(void);

/* Modifier flags (bitmask) */
typedef enum {
    KBD_SHIFT    = 1 << 0,
    KBD_CTRL     = 1 << 1,
    KBD_ALT      = 1 << 2,
    KBD_CAPSLOCK = 1 << 3,
    KBD_NUMLOCK  = 1 << 4
} kbd_flags_t;

/* Thread-safe keyboard state */
typedef struct {
    uint8_t flags;
    spinlock_t lock;
} kbd_state_t;

static kbd_state_t kbd_state = {
    .flags = 0,
    .lock = SPINLOCK_INIT()
};

/* Keymap with 4 mappings: Normal, Shift, Ctrl, Alt */
static char key_map[][4] = {
    [VT_KEY_NONE] = {'\0', '\0', '\0', '\0'},

    /* Alphanumeric keys */
    [VT_KEY_A] = {'a', 'A', CTRL('A'), 0x01},
    [VT_KEY_B] = {'b', 'B', CTRL('B'), 0x02},
    [VT_KEY_C] = {'c', 'C', CTRL('C'), 0x03},
    [VT_KEY_D] = {'d', 'D', CTRL('D'), 0x04},
    [VT_KEY_E] = {'e', 'E', CTRL('E'), 0x05},
    [VT_KEY_F] = {'f', 'F', CTRL('F'), 0x06},
    [VT_KEY_G] = {'g', 'G', CTRL('G'), 0x07},
    [VT_KEY_H] = {'h', 'H', CTRL('H'), 0x08},
    [VT_KEY_I] = {'i', 'I', CTRL('I'), 0x09},
    [VT_KEY_J] = {'j', 'J', CTRL('J'), 0x0A},
    [VT_KEY_K] = {'k', 'K', CTRL('K'), 0x0B},
    [VT_KEY_L] = {'l', 'L', CTRL('L'), 0x0C},
    [VT_KEY_M] = {'m', 'M', CTRL('M'), 0x0D},
    [VT_KEY_N] = {'n', 'N', CTRL('N'), 0x0E},
    [VT_KEY_O] = {'o', 'O', CTRL('O'), 0x0F},
    [VT_KEY_P] = {'p', 'P', CTRL('P'), 0x10},
    [VT_KEY_Q] = {'q', 'Q', CTRL('Q'), 0x11},
    [VT_KEY_R] = {'r', 'R', CTRL('R'), 0x12},
    [VT_KEY_S] = {'s', 'S', CTRL('S'), 0x13},
    [VT_KEY_T] = {'t', 'T', CTRL('T'), 0x14},
    [VT_KEY_U] = {'u', 'U', CTRL('U'), 0x15},
    [VT_KEY_V] = {'v', 'V', CTRL('V'), 0x16},
    [VT_KEY_W] = {'w', 'W', CTRL('W'), 0x17},
    [VT_KEY_X] = {'x', 'X', CTRL('X'), 0x18},
    [VT_KEY_Y] = {'y', 'Y', CTRL('Y'), 0x19},
    [VT_KEY_Z] = {'z', 'Z', CTRL('Z'), 0x1A},

    /* Number keys */
    [VT_KEY_0] = {'0', ')', '\0', '\0'},
    [VT_KEY_1] = {'1', '!', '\0', '\0'},
    [VT_KEY_2] = {'2', '@', '\0', '\0'},
    [VT_KEY_3] = {'3', '#', '\0', '\0'},
    [VT_KEY_4] = {'4', '$', '\0', '\0'},
    [VT_KEY_5] = {'5', '%', '\0', '\0'},
    [VT_KEY_6] = {'6', '^', '\0', '\0'},
    [VT_KEY_7] = {'7', '&', '\0', '\0'},
    [VT_KEY_8] = {'8', '*', '\0', '\0'},
    [VT_KEY_9] = {'9', '(', '\0', '\0'},

    /* Control and function keys */
    [VT_KEY_ESC]     = {'\e', '\e', '\e', '\0'},
    [VT_KEY_TAB]     = {'\t', '\t', '\t', '\0'},
    [VT_KEY_CAPSLOCK]= {'\0', '\0', '\0', '\0'},
    [VT_KEY_LCTRL]   = {'\0', '\0', '\0', '\0'},
    [VT_KEY_RCTRL]   = {'\0', '\0', '\0', '\0'},
    [VT_KEY_LSHIFT]  = {'\0', '\0', '\0', '\0'},
    [VT_KEY_RSHIFT]  = {'\0', '\0', '\0', '\0'},
    [VT_KEY_LALT]    = {'\0', '\0', '\0', '\0'},
    [VT_KEY_RALT]    = {'\0', '\0', '\0', '\0'},
    [VT_KEY_META]    = {'\0', '\0', '\0', '\0'},
    [VT_KEY_ENTER]   = {'\r', '\r', '\r', '\0'},
    [VT_KEY_BACKSPACE]= {'\b', '\b', '\b', '\0'},
    [VT_KEY_SPACE]   = {' ', ' ', ' ', '\0'},

    /* Function keys */
    [VT_KEY_F1]      = {'\0', '\0', '\0', '\0'},
    [VT_KEY_F2]      = {'\0', '\0', '\0', '\0'},
    [VT_KEY_F3]      = {'\0', '\0', '\0', '\0'},
    [VT_KEY_F4]      = {'\0', '\0', '\0', '\0'},
    [VT_KEY_F5]      = {'\0', '\0', '\0', '\0'},
    [VT_KEY_F6]      = {'\0', '\0', '\0', '\0'},
    [VT_KEY_F7]      = {'\0', '\0', '\0', '\0'},
    [VT_KEY_F8]      = {'\0', '\0', '\0', '\0'},
    [VT_KEY_F9]      = {'\0', '\0', '\0', '\0'},
    [VT_KEY_F10]     = {'\0', '\0', '\0', '\0'},
    [VT_KEY_F11]     = {'\0', '\0', '\0', '\0'},
    [VT_KEY_F12]     = {'\0', '\0', '\0', '\0'},

    /* Arrow and nav keys */
    [VT_KEY_UP]      = {'\0', '\0', '\0', '\0'},
    [VT_KEY_DOWN]    = {'\0', '\0', '\0', '\0'},
    [VT_KEY_LEFT]    = {'\0', '\0', '\0', '\0'},
    [VT_KEY_RIGHT]   = {'\0', '\0', '\0', '\0'},
    [VT_KEY_HOME]    = {'\0', '\0', '\0', '\0'},
    [VT_KEY_END]     = {'\0', '\0', '\0', '\0'},
    [VT_KEY_PGUP]    = {'\0', '\0', '\0', '\0'},
    [VT_KEY_PGDN]    = {'\0', '\0', '\0', '\0'},
    [VT_KEY_DEL]     = {'\0', '\0', '\0', '\0'},

    /* Punctuation and symbols */
    [VT_KEY_SEMICOLON] = {';', ':', '\0', '\0'},
    [VT_KEY_QUOTE]     = {'\'', '"', '\0', '\0'},
    [VT_KEY_BACKTICK]  = {'`', '~', '\0', '\0'},
    [VT_KEY_COMMA]     = {',', '<', '\0', '\0'},
    [VT_KEY_PERIOD]    = {'.', '>', '\0', '\0'},
    [VT_KEY_SLASH]     = {'/', '?', '\0', '\0'},
    [VT_KEY_BSLASH]    = {'\\', '|', '\0', '\0'},
    [VT_KEY_LBRACKET]  = {'[', '{', '\0', '\0'},
    [VT_KEY_RBRACKET]  = {']', '}', '\0', '\0'},
    [VT_KEY_MINUS]     = {'-', '_', '\0', '\0'},
    [VT_KEY_EQUAL]     = {'=', '+', '\0', '\0'},

    /* Keypad */
    [VT_KEY_NUMLOCK]  = {'\0', '\0', '\0', '\0'},
    [VT_KEY_SCRLOCK]  = {'\0', '\0', '\0', '\0'},
    [VT_KEY_KP_0]     = {'0', '0', '\0', '\0'},
    [VT_KEY_KP_1]     = {'1', '1', '\0', '\0'},
    [VT_KEY_KP_2]     = {'2', '2', '\0', '\0'},
    [VT_KEY_KP_3]     = {'3', '3', '\0', '\0'},
    [VT_KEY_KP_4]     = {'4', '4', '\0', '\0'},
    [VT_KEY_KP_5]     = {'5', '5', '\0', '\0'},
    [VT_KEY_KP_6]     = {'6', '6', '\0', '\0'},
    [VT_KEY_KP_7]     = {'7', '7', '\0', '\0'},
    [VT_KEY_KP_8]     = {'8', '8', '\0', '\0'},
    [VT_KEY_KP_9]     = {'9', '9', '\0', '\0'},

    /* Miscellaneous */
    [VT_KEY_INS]      = {'\0', '\0', '\0', '\0'},
    [VT_KEY_MENU]     = {'\0', '\0', '\0', '\0'},
    [VT_KEY_PRTSC]    = {'\0', '\0', '\0', '\0'},
    [VT_KEY_PAUSE]    = {'\0', '\0', '\0', '\0'},
    [VT_KEY_BREAK]    = {'\0', '\0', '\0', '\0'},
};

/* ANSI escape sequences for special keys */
static const char* ansi_sequences[] = {
    /* Navigation keys */
    [VT_KEY_UP]      = "A",        // ↑
    [VT_KEY_DOWN]    = "B",        // ↓
    [VT_KEY_LEFT]    = "D",        // ←
    [VT_KEY_RIGHT]   = "C",        // →
    [VT_KEY_HOME]    = "H",        // Home
    [VT_KEY_END]     = "F",        // End
    [VT_KEY_PGUP]    = "5~",       // Page Up
    [VT_KEY_PGDN]    = "6~",       // Page Down
    
    /* Editing keys */
    [VT_KEY_INS]     = "2~",       // Insert
    [VT_KEY_DEL]     = "3~",       // Delete
    [VT_KEY_BACKSPACE] = "\x7F",   // Backspace (DEL character)
    
    /* Function keys */
    [VT_KEY_F1]      = "11~",      // F1
    [VT_KEY_F2]      = "12~",      // F2
    [VT_KEY_F3]      = "13~",      // F3
    [VT_KEY_F4]      = "14~",      // F4
    [VT_KEY_F5]      = "15~",      // F5
    [VT_KEY_F6]      = "17~",      // F6
    [VT_KEY_F7]      = "18~",      // F7
    [VT_KEY_F8]      = "19~",      // F8
    [VT_KEY_F9]      = "20~",      // F9
    [VT_KEY_F10]     = "21~",      // F10
    [VT_KEY_F11]     = "23~",      // F11
    [VT_KEY_F12]     = "24~",      // F12
    
    /* Keypad keys (when numlock is off) */
    [VT_KEY_KP_0]    = "0x",       // Keypad 0
    [VT_KEY_KP_1]    = "1x",       // Keypad 1
    [VT_KEY_KP_2]    = "2x",       // Keypad 2
    [VT_KEY_KP_3]    = "3x",       // Keypad 3
    [VT_KEY_KP_4]    = "4x",       // Keypad 4
    [VT_KEY_KP_5]    = "5x",       // Keypad 5
    [VT_KEY_KP_6]    = "6x",       // Keypad 6
    [VT_KEY_KP_7]    = "7x",       // Keypad 7
    [VT_KEY_KP_8]    = "8x",       // Keypad 8
    [VT_KEY_KP_9]    = "9x",       // Keypad 9
    
    /* Special cases */
    [VT_KEY_ESC]     = "\x1B",     // Escape
    [VT_KEY_TAB]     = "\t",       // Tab
    [VT_KEY_ENTER]   = "\r",       // Enter (CR)
    [VT_KEY_MENU]    = "29~",      // Menu key
    [VT_KEY_PRTSC]   = "?1",       // Print Screen
    [VT_KEY_PAUSE]   = "?2",       // Pause/Break
    
    /* Modifier combinations (example for shifted keys) */
    [VT_KEY_LSHIFT]  = "",         // No sequence
    [VT_KEY_RSHIFT]  = "",         // No sequence
    [VT_KEY_LCTRL]   = "",         // No sequence
    [VT_KEY_RCTRL]   = "",         // No sequence
    [VT_KEY_LALT]    = "",         // No sequence
    [VT_KEY_RALT]    = "",         // No sequence
    [VT_KEY_META]    = "",         // No sequence
    
    /* Lock keys */
    [VT_KEY_CAPSLOCK] = "",        // No sequence
    [VT_KEY_NUMLOCK] = "",         // No sequence
    [VT_KEY_SCRLOCK] = "",         // No sequence
};

static devid_t *kbdev   = DEVID_PTR(NULL, CHRDEV, DEV_T(KBDEV_DEV_MAJOR, 0));

/* Helper macros */
#define KEY_ALT(k)      ((k) == VT_KEY_LALT   || (k) == VT_KEY_RALT)
#define KEY_CONTROL(k)  ((k) == VT_KEY_LCTRL  || (k) == VT_KEY_RCTRL)
#define KEY_SHIFT(k)    ((k) == VT_KEY_LSHIFT || (k) == VT_KEY_RSHIFT)
#define KEY_MODIFIER(k) (KEY_SHIFT(k) || KEY_CONTROL(k) || KEY_ALT(k))

/* Thread-safe flag manipulation */
static void set_kbd_flag(kbd_flags_t flag, bool state) {
    spin_lock(&kbd_state.lock);
    if (state) {
        kbd_state.flags |= flag;
    } else {
        kbd_state.flags &= ~flag;
    }
    spin_unlock(&kbd_state.lock);
}

static bool get_kbd_flag(kbd_flags_t flag) {
    spin_lock(&kbd_state.lock);
    bool state = (kbd_state.flags & flag);
    spin_unlock(&kbd_state.lock);
    return state;
}

/* Send multiple bytes to TTY */
static void send_to_tty(tty_t *tp, const char   *data, size_t len) {
    if (!tp) {
        return;
    }

    tty_lock(tp);
    if (tp->t_ldisc && tp->t_ldisc->receive_buf && data && len > 0) {
        tp->t_ldisc->receive_buf(tp, data, NULL, len);
    }
    tty_unlock(tp);
}

/* Single-character convenience wrapper */
static void send_char_to_tty(tty_t *tp, char c) {
    send_to_tty(tp, &c, 1);
}

/* Send ANSI escape sequence efficiently */
static void send_ansi_sequence(tty_t *tp, vt_key_t key) {
    if (key >= NELEM(ansi_sequences)) {
        return;
    }

    const char *seq = ansi_sequences[key];
    if (!seq || !*seq) {
        return;
    }

    /* Buffer for building the complete sequence */
    char   buf[16];  // Enough for any ANSI sequence
    size_t pos = 0;

    /* Start with ESC[ */
    buf[pos++] = '\x1B';
    buf[pos++] = '[';

    /* Append the sequence */
    while (*seq && pos < sizeof(buf) - 1) {
        buf[pos++] = *seq++;
    }

    /* Send complete sequence in one operation */
    send_to_tty(tp, buf, pos);
}

/* Key release handler */
static void key_released(vt_key_t vt_key) {
    if (KEY_SHIFT(vt_key)) {
        set_kbd_flag(KBD_SHIFT, false);
    } else if (KEY_CONTROL(vt_key)) {
        set_kbd_flag(KBD_CTRL, false);
    } else if (KEY_ALT(vt_key)) {
        set_kbd_flag(KBD_ALT, false);
    }
}

/* Main key processing function */
static void process_key(tty_t *tp, vt_key_t vt_key) {
    /* Handle modifier toggles */
    if (vt_key == VT_KEY_CAPSLOCK) {
        bool current_state = get_kbd_flag(KBD_CAPSLOCK);
        set_kbd_flag(KBD_CAPSLOCK, !current_state);
        return;
    }

    if (vt_key == VT_KEY_NUMLOCK) {
        bool current_state = get_kbd_flag(KBD_NUMLOCK);
        set_kbd_flag(KBD_NUMLOCK, !current_state);
        return;
    }

    /* Update modifier flags */
    if (KEY_SHIFT(vt_key)) {
        set_kbd_flag(KBD_SHIFT, true);
        return;
    } else if (KEY_CONTROL(vt_key)) {
        set_kbd_flag(KBD_CTRL, true);
        return;
    } else if (KEY_ALT(vt_key)) {
        set_kbd_flag(KBD_ALT, true);
        return;
    }

    /* Get current_state modifier state */
    bool alt    = get_kbd_flag(KBD_ALT);
    bool ctrl   = get_kbd_flag(KBD_CTRL);
    bool shift  = get_kbd_flag(KBD_SHIFT);
    bool caps   = get_kbd_flag(KBD_CAPSLOCK);

    /* Handle special key combinations */
    if (ctrl && shift) {
        switch (vt_key) {
            case VT_KEY_M:
                toggle_sched_monitor();
                return;
            case VT_KEY_S:
                toggle_kernel_shell();
                return;
            /* TODO: add more Ctrl+Shift combinations as needed */
            default: return;
        }
    }

    /* Get the appropriate key mapping */
    char key = '\0';
    if (alt && key_map[vt_key][3] != '\0') {
        key = key_map[vt_key][3];  // Alt mapping
    } else if (ctrl && key_map[vt_key][2] != '\0') {
        key = key_map[vt_key][2];  // Ctrl mapping
    } else if ((shift || caps) && key_map[vt_key][1] != '\0') {
        key = key_map[vt_key][1];  // Shift mapping
    } else {
        key = key_map[vt_key][0];  // Normal mapping
    }

    /* Handle special cases */
    if (key == '\0') {
        if (vt_key >= VT_KEY_UP && vt_key <= VT_KEY_DEL) {
            send_ansi_sequence(tp, vt_key);
        } else if (vt_key >= VT_KEY_F1 && vt_key <= VT_KEY_F12) {
            send_ansi_sequence(tp, vt_key);
        }
        return;
    }

    send_char_to_tty(tp, key);
}

void tty_receive_input(tty_t *tp, void *data) {
    kbd_event_t *event = data;

    if (tp == NULL || event == NULL) {
        return;
    }

    if (event->vt_key == VT_KEY_NONE || event->vt_key >= VT_KEY_MAX) {
    }

    if (event->ev_flags & EV_MAKE) {
        process_key(tp, event->vt_key);
    } else {
        key_released(event->vt_key);
    }
}

/* Main input thread */
void tty_input(void) {
    tty_t *tp = NULL;
    kbd_event_t event;

    while (device_open(kbdev, NULL)) { // try to open the keyboard event.
        sched_yield();
    }
    
    while (1) {
        tp = atomic_load(&active_tty);
        if (tp == NULL) {
            sched_yield();
            continue;
        }

        if (device_ioctl(kbdev, EV_READ, &event) != 0) {
            sched_yield();
            continue;
        }

        if (event.vt_key == VT_KEY_NONE || event.vt_key >= VT_KEY_MAX) {
            continue;
        }

        if (event.ev_flags & EV_MAKE) {
            process_key(tp, event.vt_key);
        } else {
            key_released(event.vt_key);
        }
    }
}

// BUILTIN_THREAD(tty_input, (thread_entry_t)tty_input, NULL);