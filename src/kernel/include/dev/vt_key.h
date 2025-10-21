#pragma once

#include <core/types.h>

// ────────────── Virtual Key Codes ──────────────
typedef enum {
    VT_KEY_UNDEF = 0,

    // Alphanumeric keys
    VT_KEY_A, VT_KEY_B, VT_KEY_C, VT_KEY_D, VT_KEY_E, VT_KEY_F,
    VT_KEY_G, VT_KEY_H, VT_KEY_I, VT_KEY_J, VT_KEY_K, VT_KEY_L,
    VT_KEY_M, VT_KEY_N, VT_KEY_O, VT_KEY_P, VT_KEY_Q, VT_KEY_R,
    VT_KEY_S, VT_KEY_T, VT_KEY_U, VT_KEY_V, VT_KEY_W, VT_KEY_X,
    VT_KEY_Y, VT_KEY_Z, VT_KEY_0, VT_KEY_1, VT_KEY_2, VT_KEY_3,
    VT_KEY_4, VT_KEY_5, VT_KEY_6, VT_KEY_7, VT_KEY_8, VT_KEY_9,

    // Control and function keys
    VT_KEY_ESC,     VT_KEY_TAB,     VT_KEY_CAPSLOCK,
    VT_KEY_LCTRL,   VT_KEY_RCTRL,   VT_KEY_LSHIFT, VT_KEY_RSHIFT,
    VT_KEY_LALT,    VT_KEY_RALT,    VT_KEY_META,
    VT_KEY_ENTER,   VT_KEY_BACKSPACE, VT_KEY_SPACE,
    VT_KEY_F1,      VT_KEY_F2,      VT_KEY_F3,  VT_KEY_F4,
    VT_KEY_F5,      VT_KEY_F6,      VT_KEY_F7,  VT_KEY_F8,
    VT_KEY_F9,      VT_KEY_F10,     VT_KEY_F11, VT_KEY_F12,

    // Arrow and nav keys
    VT_KEY_UP,      VT_KEY_DOWN, VT_KEY_LEFT, VT_KEY_RIGHT,
    VT_KEY_HOME,    VT_KEY_END,  VT_KEY_PGUP, VT_KEY_PGDN,
    VT_KEY_DEL,

    // Punctuation and symbols
    VT_KEY_SEMICOLON, VT_KEY_QUOTE,     VT_KEY_BACKTICK,
    VT_KEY_COMMA,     VT_KEY_PERIOD,    VT_KEY_SLASH,     
    VT_KEY_BSLASH,    VT_KEY_LBRACKET,  VT_KEY_RBRACKET,
    VT_KEY_MINUS,     VT_KEY_EQUAL,

    // Keypad
    VT_KEY_NUMLOCK, VT_KEY_SCRLOCK,
    VT_KEY_KP_0,    VT_KEY_KP_1, VT_KEY_KP_2,  VT_KEY_KP_3,  VT_KEY_KP_4,
    VT_KEY_KP_5,    VT_KEY_KP_6, VT_KEY_KP_7,  VT_KEY_KP_8,  VT_KEY_KP_9,

    VT_KEY_INS,     VT_KEY_MENU, VT_KEY_PRTSC, VT_KEY_PAUSE, VT_KEY_BREAK,

    // Add more as needed…
    VT_KEY_MAX,
} vt_key_t;

