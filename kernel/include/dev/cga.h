#pragma once

#include <stddef.h>

extern int use_cga;

// CGA color codes
enum cga_colors {
    CGA_BLACK   = 0,
    CGA_BLUE    = 1,
    CGA_GREEN   = 2,
    CGA_CYAN    = 3,
    CGA_RED     = 4,
    CGA_MAGENTA = 5,
    CGA_BROWN   = 6,
    CGA_LGREY   = 7,
    CGA_DGREY   = 8,
    CGA_LBLUE   = 9,
    CGA_LGREEN  = 10,
    CGA_LCYAN   = 11,
    CGA_LRED    = 12,
    CGA_LMAGENTA= 13,
    CGA_YELLOW  = 14,
    CGA_WHITE   = 15
};

#define CGA_RESET       "\e[0m"

#define CGA_FG_BLACK    "\e[30m"
#define CGA_FG_BLUE     "\e[31m"
#define CGA_FG_GREEN    "\e[32m"
#define CGA_FG_CYAN     "\e[33m"
#define CGA_FG_RED      "\e[34m"
#define CGA_FG_MAGENTA  "\e[35m"
#define CGA_FG_BROWN    "\e[36m"
#define CGA_FG_LGREY    "\e[37m"

#define CGA_BG_BLACK    "\e[40m"
#define CGA_BG_BLUE     "\e[41m"
#define CGA_BG_GREEN    "\e[42m"
#define CGA_BG_CYAN     "\e[43m"
#define CGA_BG_RED      "\e[44m"
#define CGA_BG_MAGENTA  "\e[45m"
#define CGA_BG_BROWN    "\e[46m"
#define CGA_BG_LGREY    "\e[47m"

extern int      cga_init(void);
extern void     cga_putc(const int);
extern size_t   cga_puts(const char *s);
extern void     cga_putchar_at(const int c, int x, int y);
extern void     cga_puts_at(const char *buf, int x, int y);