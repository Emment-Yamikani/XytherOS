#include <arch/x86_64/asm.h>
#include <core/defs.h>
#include <core/types.h>
#include <dev/cga.h>
#include <ds/stack.h>
#include <lib/ctype.h>
#include <mm/mem.h>
#include <string.h>

#define CGA_WIDTH        80
#define CGA_HEIGHT       25
#define CGA_SCREEN_SIZE  (CGA_WIDTH * CGA_HEIGHT)
#define CGA_ADDRESS      0xB8000

static volatile bool    cga_active  = 0;
static uint16_t         *cga_mem    = (uint16_t *)V2HI(CGA_ADDRESS);

// Color state structure
typedef union {
    struct {
        uint8_t _;
        uint8_t fg : 4;
        uint8_t bg : 4;
    };
    uint16_t attrib;
} color_state;

// Current text attributes and stack
static int          stack_top     = 0;
static uint16_t     cursor_pos    = 0;
static uint16_t     saved_cursor_pos = 0;
static color_state  color_scheme  = { .fg = CGA_LGREY, .bg = CGA_BLACK }; // Light grey on black (default)

#define COLOR_STACK_SIZE 1024
static color_state  color_stack[COLOR_STACK_SIZE];
static const color_state default_color = { .fg = CGA_LGREY, .bg = CGA_BLACK };

#define CGA_BLANK_SPACE (color_scheme.attrib | ' ')

// Push color_scheme color state to stack
void push_color() {
    if (stack_top < COLOR_STACK_SIZE) {
        color_stack[++stack_top] = color_scheme;
    }
}

// Pop color state from stack
void pop_color() {
    if (stack_top >= 0) {
        color_scheme = color_stack[stack_top--];
    } else {
        // Reset to default if stack is empty
        color_scheme = default_color;
    }
}

void cga_setcursor(int __pos) {
    if (__pos < 0) __pos = 0;
    if (__pos >= CGA_SCREEN_SIZE) __pos = CGA_SCREEN_SIZE - 1;

    cursor_pos = __pos;
    outb(0x3d4, 14);
    outb(0x3d5, (AND(SHR(__pos, 8), 0xFF)));
    outb(0x3d4, 15);
    outb(0x3d5, (__pos & 0xFF));
}

void cga_reset_cursor(void) {
    cga_setcursor(0);
}

void cga_scroll(void) {
    // Scroll up
    memmove((void *)cga_mem,
            (void *)V2HI(CGA_ADDRESS + CGA_WIDTH * 2),
            CGA_WIDTH * (CGA_HEIGHT - 1) * 2);

    // Clear bottom line
    for (int i = 0; i < CGA_WIDTH; i++) {
        cga_mem[(CGA_HEIGHT - 1) * CGA_WIDTH + i] = CGA_BLANK_SPACE;
    }

    cursor_pos = (CGA_HEIGHT - 1) * CGA_WIDTH;
}

void cga_clr(void) {
    memsetw(cga_mem, CGA_BLANK_SPACE, CGA_SCREEN_SIZE);
    cga_reset_cursor();
}

int cga_init(void) {
    cga_clr();
    return 0;
}

static inline int cga_xy2linear(int x, int y) {
    return y * CGA_WIDTH + x;
}

typedef struct Point { int x; int y; } Point;

static inline int cga_point2linear(Point point) {
    return point.y * CGA_WIDTH + point.x;
}

static inline Point cga_linear2point(int pos) {
    return (Point){pos % CGA_WIDTH, pos / CGA_WIDTH};
}

// set graphics rendition
void cga_handle_sgr(const int code) {
    // Save previous color state before any changes
    if (code != 0) push_color();

    switch (code) {
        case 0: // Reset
            pop_color();
            break;
        case 1:  // Bold/Bright foreground
            color_scheme.fg |= 0x08;
            break;
        case 22: // Normal intensity
            color_scheme.fg &= ~0x08;
            break;
        case 30 ... 37:  // Standard foreground
            color_scheme.fg = (color_scheme.fg & 0xF8) | (code - 30);
            break;
        case 39: // Default foreground
            color_scheme.fg = 0x07;
            break;
        case 40 ... 47:  // Standard background
            color_scheme.bg = (code - 40);
            break;
        case 49: // Default background
            color_scheme.bg = 0x00;
            break;
        case 90 ... 97:  // Bright foreground
            color_scheme.fg = (code - 90) | 0x08;
            break;
        case 100 ... 107: // Bright background
            color_scheme.bg = ((code - 100) | 0x08);
            break;
        default:
            break;  // Ignore unsupported codes
    }
}

void cga_handle_cursor_movement(int cmd, int param) {
    // Default to 1 if param is 0 (ANSI behavior)
    if (param <= 0) param = 1;
    
    int x = cursor_pos % CGA_WIDTH;
    int y = cursor_pos / CGA_WIDTH;

    switch (cmd) {
        case 'A': // CUU - Cursor Up
            y -= param;
            break;
            
        case 'B': // CUD - Cursor Down
            y += param;
            break;
            
        case 'C': // CUF - Cursor Forward (Right)
            x += param;
            break;
            
        case 'D': // CUB - Cursor Back (Left)
            x -= param;
            break;
    }

    // Apply bounds checking
    x = (x < 0) ? 0 : (x >= CGA_WIDTH) ? CGA_WIDTH - 1 : x;
    y = (y < 0) ? 0 : (y >= CGA_HEIGHT) ? CGA_HEIGHT - 1 : y;

    // Update cursor position
    cga_setcursor(cga_xy2linear(x, y));

}

// Set cursor position with bounds checking
void cga_set_cursor_position(int x, int y) {
    // Apply bounds checking
    x = (x < 0) ? 0 : (x >= CGA_WIDTH) ? CGA_WIDTH - 1 : x;
    y = (y < 0) ? 0 : (y >= CGA_HEIGHT) ? CGA_HEIGHT - 1 : y;

    cga_setcursor(cga_xy2linear(x, y));
}

// Handle erase operations
void cga_handle_erase(int code) {
    Point position = cga_linear2point(cursor_pos);

    switch (code) {
        case 0:  // Erase from cursor to end of screen
            int length = CGA_SCREEN_SIZE - cursor_pos;
            memsetw(&cga_mem[cursor_pos], CGA_BLANK_SPACE, length);
            break;
        case 1:  // Erase from beginning to cursor
            cga_reset_cursor();
            length = cga_point2linear(position) - cursor_pos;
            memsetw(&cga_mem[cursor_pos], CGA_BLANK_SPACE, length);
            break;
        case 2:  // Erase entire screen
            cga_clr();
            break;
    }
}

void cga_handle_erase_line(int code) {
    Point position = cga_linear2point(cursor_pos);

    switch (code) {
        case 0:  // Erase from cursor to end of line
            int length = cga_xy2linear(CGA_WIDTH - 1, position.y) -  cursor_pos;
            memsetw(&cga_mem[cursor_pos], CGA_BLANK_SPACE, length);
            break;
        case 1:  // Erase from beginning to cursor
            cga_setcursor(cga_xy2linear(0, position.y));
            length = cga_point2linear(position) - cursor_pos;
            memsetw(&cga_mem[cursor_pos], CGA_BLANK_SPACE, length);
            break;
        case 2:  // Erase entire line
            cga_setcursor(cga_xy2linear(0, position.y));
            length = CGA_WIDTH;
            memsetw(&cga_mem[cursor_pos], CGA_BLANK_SPACE, length);
            break;
    }
}

// Write a character to CGA memory
void cga_putc(const int c) {

    // Handle escape sequences
    static int esc_state = 0;
    static int esc_value = 0;
    static int esc_values[4];
    static int esc_index = 0;

    if (esc_state == 0 && c == '\x1B') {
        esc_state = 1; // ESC detected
        return;
    }

    if (esc_state == 1) {
        if (c == '[') {
            esc_state = 2; // CSI sequence
            esc_value = 0;
            esc_index = 0;
            memset(esc_values, 0, sizeof(esc_values));
            return;
        } else {
            esc_state = 0; // Not a CSI sequence
        }
    }

    if (esc_state == 2) {
        if (c >= '0' && c <= '9') {
            esc_value = esc_value * 10 + (c - '0');
        } else if (c == ';') {
            if (esc_index < 3) {
                esc_values[esc_index++] = esc_value;
                esc_value = 0;
            }
        } else {
            // Final character of CSI sequence - process command
            esc_values[esc_index] = esc_value;

            switch (c) {
                case 'm':  // SGR - Select Graphic Rendition
                    for (int i = 0; i <= esc_index; i++) {
                        cga_handle_sgr(esc_values[i]);
                    }
                    esc_state = 0;
                    return;

                case 'A' ... 'D':  // Cursor movement
                    // Default to 1 if no parameter specified
                    int param = (esc_index >= 0) ? esc_values[0] : 1;
                    cga_handle_cursor_movement(c, param);
                    esc_state = 0;
                    return;

                case 'H': case 'f':  // Cursor Position
                    // Format: ESC [ <row> ; <col> H
                    // Defaults to 1,1 if no parameters
                    int row = (esc_index >= 0) ? esc_values[0] : 1;
                    int col = (esc_index >= 1) ? esc_values[1] : 1;
                    cga_set_cursor_position(col - 1, row - 1);  // Convert to 0-based
                    esc_state = 0;
                    return;

                case 's':  // Save cursor position
                    saved_cursor_pos = cursor_pos;
                    esc_state = 0;
                    return;

                case 'u':  // Restore cursor position
                    cga_setcursor(saved_cursor_pos);
                    esc_state = 0;
                    return;

                case 'G':  // Cursor Horizontal Absolute
                    // Default to column 1 if no parameter
                    int column   = (esc_index >= 0) ? esc_values[0] : 1;
                    int cursor_y = cursor_pos / CGA_WIDTH;
                    cga_set_cursor_position(column - 1, cursor_y);  // 0-based
                    esc_state = 0;
                    return;

                case 'd':  // Vertical Position Absolute
                    // Default to row 1 if no parameter
                    int row_vpa = (esc_index >= 0) ? esc_values[0] : 1;
                    int cursor_x = cursor_pos % CGA_WIDTH;
                    cga_set_cursor_position(cursor_x, row_vpa - 1);  // 0-based
                    esc_state = 0;
                    return;

                case 'E':  // Cursor Next Line
                    // Default to 1 if no parameter
                    int lines_down = (esc_index >= 0) ? esc_values[0] : 1;
                    cursor_y = cursor_pos / CGA_WIDTH;
                    cga_set_cursor_position(0, cursor_y + lines_down);
                    esc_state = 0;
                    return;

                case 'F':  // Cursor Previous Line
                    // Default to 1 if no parameter
                    int lines_up = (esc_index >= 0) ? esc_values[0] : 1;
                    cursor_y = cursor_pos / CGA_WIDTH;
                    cga_set_cursor_position(0, cursor_y - lines_up);
                    esc_state = 0;
                    return;

                case 'J':  // Erase in Display
                    cga_handle_erase(esc_values[0]);
                    esc_state = 0;
                    return;

                case 'K':  // Erase in Line
                    cga_handle_erase_line(esc_values[0]);
                    esc_state = 0;
                    return;

                default:
                    // Unsupported or invalid CSI sequence
                    break;
            }

            esc_state = 0; // Invalid sequence
        }
    }

    // Handle normal characters
    if (esc_state == 0) {
        switch (c) {
            case '\n':
                // Newline - move to start of next line
                cursor_pos = ((cursor_pos / CGA_WIDTH) + 1) * CGA_WIDTH;
                break;
            case '\r':
                // Carriage return - move to start of line
                cursor_pos = (cursor_pos / CGA_WIDTH) * CGA_WIDTH;
                break;
            case '\t':
                // Tab - advance to next tab stop (every 8 chars)
                cursor_pos = (cursor_pos + 8) & ~7;
                break;
            case '\b':
                // Backspace - move cursor back one position
                if (cursor_pos > 0) cursor_pos--;
                break;
            case ' ' ... (u8)'\xff':
                // Printable character
                cga_mem[cursor_pos++] = color_scheme.attrib | c;
        }
        // Handle screen scrolling
        if (cursor_pos >= CGA_WIDTH * CGA_HEIGHT) {
            cga_scroll();
        }

        cga_setcursor(cursor_pos);
    }
}

size_t cga_puts(const char *s) {
    char *S = (char *)s;
    static spinlock_t lock = SPINLOCK_INIT();

    spin_lock(&lock);
    while (S && *S)
        cga_putc(*S++);
    spin_unlock(&lock);
    return (size_t)(S - s);
}