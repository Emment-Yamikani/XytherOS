#pragma once

#include <core/types.h>
#include <ds/stack.h>
#include <dev/clocks.h>
#include <font/tinyfont.h>
#include <fs/inode.h>
#include <sync/spinlock.h>

#ifndef _SC_UP
    #define _SC_UP      0xE2
#endif

#ifndef _SC_LEFT
    #define _SC_LEFT    0xE4
#endif

#ifndef _SC_RIGHT
    #define _SC_RIGHT   0xE5
#endif

#ifndef _SC_DOWN
    #define _SC_DOWN    0xE3
#endif


typedef u32 pixel_t;

typedef struct {
    char        cursor;
    int         cursor_timeout;

    int         c_col;
    int         c_row;
    int         c_ncols;
    int         c_nrows;

    bool        escape;
    bool        transp;
    bool        use_wall;
    int         opacity;

    int         width;
    int         height;
    pixel_t     fg_color;
    pixel_t     bg_color;
    pixel_t     *bg_img;

    pixel_t     *backbuffer;
    pixel_t     **scanline0;

    pixel_t     *frontbuffer;
    pixel_t     **scanline1;

    clockid_t   clkid;
    struct font *font;
    inode_t     *wallpaper;

    stack_t     char_stack;
    stack_t     theme_stack;

    spinlock_t  lock;
} limeterm_ctx_t;

#define DEFAULT_BG  RGB_black
#define DEFAULT_FG  RGB_antique_white

#define DEFAULT_CURSOR  '|'
#define DEFAULT_TIMEOUT 100000 // 100000us

#define DEFAULT_OPACITY 200
#define DEFAULT_WALLPAPER "/ramfs/wall.jpg"