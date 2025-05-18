#include <arch/chipset.h>
#include <arch/traps.h>
#include <arch/x86_64/asm.h>
#include <bits/errno.h>
#include <dev/dev.h>
#include <dev/ps2.h>
#include <dev/tty.h>
#include <dev/kbd_event.h>
#include <ds/ringbuf.h>
#include <sys/thread.h>
#include <sys/schedule.h>

static const vt_key_t e0_scode_map[128] = {
    [0x1C] = VT_KEY_ENTER,     // Numpad Enter
    [0x1D] = VT_KEY_RCTRL,
    [0x35] = VT_KEY_SLASH,     // KP /
    [0x38] = VT_KEY_RALT,
    [0x47] = VT_KEY_HOME,
    [0x48] = VT_KEY_UP,
    [0x49] = VT_KEY_PGUP,
    [0x4B] = VT_KEY_LEFT,
    [0x4D] = VT_KEY_RIGHT,
    [0x4F] = VT_KEY_END,
    [0x50] = VT_KEY_DOWN,
    [0x51] = VT_KEY_PGDN,
    [0x52] = VT_KEY_INS,
    [0x53] = VT_KEY_DEL,
    [0x5B] = VT_KEY_META,
    [0x5C] = VT_KEY_META,
    [0x5D] = VT_KEY_MENU,
};

static const vt_key_t scode_map[128] = {
    [0x00] = VT_KEY_NONE,
    [0x01] = VT_KEY_ESC,
    [0x02] = VT_KEY_1,
    [0x03] = VT_KEY_2,
    [0x04] = VT_KEY_3,
    [0x05] = VT_KEY_4,
    [0x06] = VT_KEY_5,
    [0x07] = VT_KEY_6,
    [0x08] = VT_KEY_7,
    [0x09] = VT_KEY_8,
    [0x0A] = VT_KEY_9,
    [0x0B] = VT_KEY_0,
    [0x0C] = VT_KEY_MINUS,       // -
    [0x0D] = VT_KEY_EQUAL,        // =
    [0x0E] = VT_KEY_BACKSPACE,
    [0x0F] = VT_KEY_TAB,

    [0x10] = VT_KEY_Q,
    [0x11] = VT_KEY_W,
    [0x12] = VT_KEY_E,
    [0x13] = VT_KEY_R,
    [0x14] = VT_KEY_T,
    [0x15] = VT_KEY_Y,
    [0x16] = VT_KEY_U,
    [0x17] = VT_KEY_I,
    [0x18] = VT_KEY_O,
    [0x19] = VT_KEY_P,
    [0x1A] = VT_KEY_LBRACKET,     // [
    [0x1B] = VT_KEY_RBRACKET,     // ]
    [0x1C] = VT_KEY_ENTER,
    [0x1D] = VT_KEY_LCTRL,
    [0x1E] = VT_KEY_A,
    [0x1F] = VT_KEY_S,

    [0x20] = VT_KEY_D,
    [0x21] = VT_KEY_F,
    [0x22] = VT_KEY_G,
    [0x23] = VT_KEY_H,
    [0x24] = VT_KEY_J,
    [0x25] = VT_KEY_K,
    [0x26] = VT_KEY_L,
    [0x27] = VT_KEY_SEMICOLON,    // ;
    [0x28] = VT_KEY_QUOTE,        // '
    [0x29] = VT_KEY_BACKTICK,        // `
    [0x2A] = VT_KEY_LSHIFT,
    [0x2B] = VT_KEY_BSLASH,       // '\'
    [0x2C] = VT_KEY_Z,
    [0x2D] = VT_KEY_X,
    [0x2E] = VT_KEY_C,
    [0x2F] = VT_KEY_V,

    [0x30] = VT_KEY_B,
    [0x31] = VT_KEY_N,
    [0x32] = VT_KEY_M,
    [0x33] = VT_KEY_COMMA,
    [0x34] = VT_KEY_PERIOD,
    [0x35] = VT_KEY_SLASH,
    [0x36] = VT_KEY_RSHIFT,
    [0x37] = VT_KEY_NONE,         // PrintScreen (needs E0)
    [0x38] = VT_KEY_LALT,
    [0x39] = VT_KEY_SPACE,
    [0x3A] = VT_KEY_CAPSLOCK,
    [0x3B] = VT_KEY_F1,
    [0x3C] = VT_KEY_F2,
    [0x3D] = VT_KEY_F3,
    [0x3E] = VT_KEY_F4,
    [0x3F] = VT_KEY_F5,

    [0x40] = VT_KEY_F6,
    [0x41] = VT_KEY_F7,
    [0x42] = VT_KEY_F8,
    [0x43] = VT_KEY_F9,
    [0x44] = VT_KEY_F10,
    [0x45] = VT_KEY_NUMLOCK,
    [0x46] = VT_KEY_SCRLOCK,     // Scroll Lock
    [0x47] = VT_KEY_KP_7,
    [0x48] = VT_KEY_KP_8,
    [0x49] = VT_KEY_KP_9,
    [0x4A] = VT_KEY_MINUS,       // KP -
    [0x4B] = VT_KEY_KP_4,
    [0x4C] = VT_KEY_KP_5,
    [0x4D] = VT_KEY_KP_6,
    [0x4E] = VT_KEY_NONE,         // KP +
    [0x4F] = VT_KEY_KP_1,

    [0x50] = VT_KEY_KP_2,
    [0x51] = VT_KEY_KP_3,
    [0x52] = VT_KEY_KP_0,
    [0x53] = VT_KEY_PERIOD,      // KP .

    // TODO: Add remaining scancodes...
};

DECL_DEVOPS(static, ps2kbd);
static DECL_DEVICE(ps2kbd, FS_CHR, KBD0_DEV_MAJOR, 0);

static bool saw_e0      = false;
static int  e1_state    = 0;
static int  prtsc_state = 0;

void ps2kbd_intr(void) {
    uchar       scode;
    uchar       code;
    ev_flag_t   flags;
    vt_key_t    vt_key;
    bool        is_brk;

    flags   = EV_NONE;
    vt_key  = VT_KEY_NONE;

    scode   = inb(PS2_DATA_PORT);
    code    = scode & 0x7F;
    is_brk  = scode & 0x80 ? true : false;

    // --- Handle prefix bytes ---
    if (scode == 0xE0) {
        saw_e0 = true;
        return;
    } else if (scode == 0xE1) {
        e1_state = 1;
        return;
    }

    // --- Handle Pause sequence (E1 1D 45 E1 9D C5) ---
    if (e1_state > 0) {
        switch (e1_state) {
            case 1: // expect 0x1D
                if (scode == 0x1D) {
                    e1_state = 2;
                    return;
                }
                break;
            case 2: // expect 0x45
                if (scode == 0x45) {
                    e1_state = 3;
                    vt_key   = VT_KEY_PAUSE;
                    flags    = EV_MAKE;
                    break; // emit make now
                }
                break;
            case 3: // expect E1
                if (scode == 0xE1) {
                    e1_state = 4;
                    return;
                }
                break;
            case 4: // expect 0x9D
                if (scode == 0x9D) {
                    e1_state = 5;
                    return;
                }
                break;
            case 5: // expect 0xC5
                // No explicit break for Pause, release is implied
                e1_state = 0;
                return;
        }

        // If we get unexpected byte
        e1_state = 0;
    }

    // --- Handle extended (E0-prefixed) keys ---
    if (saw_e0) {
        saw_e0 = false;

        switch (scode) {
            case 0x2A: // E0 2A (fake shift) — start of PrtSc press
                prtsc_state = 1;
                return;
            case 0x37: // E0 37 — PrtSc press
                if (prtsc_state == 1) {
                    prtsc_state = 0;
                    vt_key      = VT_KEY_PRTSC;
                    flags       = EV_MAKE | EV_EXTENDED;
                    break;
                }
                // else: not part of PrtSc — fall through
                break;
            case 0xB7: // E0 B7 — PrtSc release
                prtsc_state = 2;
                return;
            case 0xAA: // E0 AA — end of PrtSc release
                if (prtsc_state == 2) {
                    prtsc_state = 0;
                    vt_key      = VT_KEY_PRTSC;
                    flags       = EV_EXTENDED;
                    break;
                }
                break;
            default:
                vt_key = e0_scode_map[code];  // Only use if not part of PrtSc
                flags  = EV_EXTENDED;
                flags  |= !is_brk ? EV_MAKE : 0;
                break;
        }
    } else {
        vt_key  = scode_map[code];
        flags   = !is_brk ? EV_MAKE : 0;
    }

    if (vt_key == VT_KEY_NONE) {
        return;
    }

    kbd_event_t kbd_event = {
        .ev_scode       = scode,
        .ev_flags       = flags,
        .ev_vt_key      = vt_key,
        .ev_timestamp   = epoch_get()
    };

    async_kbd_inject_event(&kbd_event);
}

static int ps2kbd_probe(devid_t *dd __unused) {

    // Disable PS/2 port 1
    ps2_send_command(PS2_CMD_DISABLE_PORT1);

    // Perform controller self-test
    if (!ps2_self_test()) {
        return -EINVAL;
    }

    // Perform interface test for port 1
    if (!ps2_interface_test()) {
        return -EINVAL;
    }

    // Re-enable PS/2 port 1
    ps2_send_command(PS2_CMD_ENABLE_PORT1);

    // Reset the keyboard
    if (!ps2kbd_reset()) {
        return -EINVAL;
    }

    // Enable keyboard scanning
    if (!ps2kbd_enable_scanning()) {
        return -EINVAL;
    }

    // enable interrupt on line 1.
    enable_intr_line(IRQ_PS2_KBD, getcpuid());

    return 0;
}

static int ps2kbd_fini(struct devid *dd __unused) {
    return 0;
}

int ps2kbd_init(void) {
    return device_register(&ps2kbddev);
}

static int ps2kbd_close(struct devid *dd __unused) {
    return 0;
}

static int ps2kbd_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int ps2kbd_mmap(struct devid *dd __unused, vmr_t *region __unused) {
    return -ENOTSUP;
}

static int ps2kbd_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOSYS;
}

static off_t ps2kbd_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -ENOTSUP;
}

static int ps2kbd_ioctl(struct devid *dd __unused, int request __unused, void *arg __unused) {
    return -ENOSYS;
}

static ssize_t ps2kbd_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t nb __unused) {
    return -ENOSYS;
}

static ssize_t ps2kbd_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t nb __unused) {
    return -ENOSYS;
}