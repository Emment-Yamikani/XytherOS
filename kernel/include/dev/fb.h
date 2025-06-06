#ifndef _DEV_FB_H
#define _DEV_FB_H 1

#include <dev/dev.h>
#include <core/types.h>
#include <sync/spinlock.h>

#define NFBDEV  8

#define FBIOGET_FIX_INFO  0x0000
#define FBIOGET_VAR_INFO  0x0001

struct fb_bitfield {
    uint8_t offset;    // position in pixel
    uint8_t length;    // length of bitfield
    uint8_t msb_right; // true if most significant byte is right
};

typedef struct fb_fixinfo {
    char        id[64];     // indentification
    int         accel;      // type of acceleration card in use
    uint32_t    type;       // type of framebuffer
    uint32_t    caps;       // capabilities
    size_t      memsz;      // total memory size of framebuffer
    uintptr_t   addr;       // physical address of framebuffer
    size_t      line_length;// bytes per line
} fb_fixinfo_t;

typedef struct fb_varinfo {
    int         bpp;        // bits per pixel
    int         width;      // pixels per row
    int         height;     // pixels per column
    uint32_t    vmode;      // video mode
    uint32_t    pitch;      // pitch
    uint32_t    grayscale;  // greyscaling
    uint32_t    colorspace; // colorspace (e.g, RBGA, e.t.c)

    /* bitfield if true color */

    struct fb_bitfield red;
    struct fb_bitfield blue;
    struct fb_bitfield green;
    struct fb_bitfield transp;
} fb_varinfo_t;

typedef struct framebuffer {
    uint32_t    id;
    device_t    *dev;
    void        *priv;
    fb_fixinfo_t *fixinfo;
    fb_varinfo_t *varinfo;
    spinlock_t   lock;
    void *module;
} framebuffer_t;

extern fb_fixinfo_t fix_info;
extern fb_varinfo_t var_info;
extern int earlycons_use_gfx;

int framebuffer_process_info();
int framebuffer_gfx_init(void);

#endif //_DEV_FB_H