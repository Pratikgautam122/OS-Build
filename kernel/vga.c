#include "vga.h"
#include "io.h"
#include <stdarg.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile unsigned short*)0xB8000)
#define COM1_PORT 0x3F8

static int cursor_x = 0;
static int cursor_y = 0;
static unsigned char text_color = 0x0F; // Bright white on black

// Serial port helpers
static void init_serial(void) {
    outb(COM1_PORT + 1, 0x00);    // Disable all interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 0, 0x03);    // Set divisor to 3 (115200 / 3 = 38400 baud)
    outb(COM1_PORT + 1, 0x00);    //                  (hi byte)
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

static int is_transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20;
}

static void write_serial(char c) {
    int timeout = 10000;
    while (is_transmit_empty() == 0 && timeout > 0) {
        timeout--;
    }
    outb(COM1_PORT, c);
}

// Update hardware cursor
static void update_cursor(void) {
    unsigned short position = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((position >> 8) & 0xFF));
}

#define TOP_BAR_ROW 0
#define BOTTOM_BAR_ROW 23
#define SCREEN_START_ROW 1
#define SCREEN_END_ROW 22

static int mystrlen(const char* s) {
    int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

static void draw_top_bar(void) {
    // Light Gray background, Black text (0x70)
    unsigned char bar_color = 0x70; 
    unsigned short blank = ' ' | (bar_color << 8);
    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[x] = blank;
    }
    
    const char* title = "  Nidhogg OS v1.0.0  ";
    const char* center = "32-bit Protected Mode Kernel";
    const char* right = "Active  ";
    
    int len = mystrlen(title);
    for (int i = 0; i < len; i++) {
        VGA_MEMORY[i] = title[i] | (bar_color << 8);
    }
    
    int center_len = mystrlen(center);
    int center_start = (VGA_WIDTH - center_len) / 2;
    for (int i = 0; i < center_len; i++) {
        VGA_MEMORY[center_start + i] = center[i] | (bar_color << 8);
    }
    
    int right_len = mystrlen(right);
    int right_start = VGA_WIDTH - right_len;
    for (int i = 0; i < right_len; i++) {
        VGA_MEMORY[right_start + i] = right[i] | (bar_color << 8);
    }
}

static void draw_bottom_bar(void) {
    // Blue background, White text (0x1F)
    unsigned char bar_color = 0x1F; 
    unsigned short blank = ' ' | (bar_color << 8);
    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[BOTTOM_BAR_ROW * VGA_WIDTH + x] = blank;
    }
    
    const char* help_text = "  Commands: help | clear | info | threads | procs | heap | mouse | trace";
    int len = mystrlen(help_text);
    for (int i = 0; i < len; i++) {
        VGA_MEMORY[BOTTOM_BAR_ROW * VGA_WIDTH + i] = help_text[i] | (bar_color << 8);
    }
}

void vga_init(void) {
    init_serial();
    vga_clear();
}

void vga_clear(void) {
    unsigned char bg = (text_color >> 4) & 0x0F;
    unsigned char clear_color = (bg << 4) | 0x0F;
    unsigned short blank = ' ' | (clear_color << 8);
    
    for (int y = SCREEN_START_ROW; y <= SCREEN_END_ROW; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = blank;
        }
    }
    
    // Clear row 24 to black/blank to prevent garbage if visible
    unsigned short black_blank = ' ' | (0x0F << 8);
    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[24 * VGA_WIDTH + x] = black_blank;
    }
    
    draw_top_bar();
    draw_bottom_bar();
    
    cursor_x = 0;
    cursor_y = SCREEN_START_ROW;
    update_cursor();
}

static void scroll(void) {
    if (cursor_y > SCREEN_END_ROW) {
        unsigned char bg = (text_color >> 4) & 0x0F;
        unsigned char clear_color = (bg << 4) | 0x0F;
        unsigned short blank = ' ' | (clear_color << 8);
        
        for (int y = SCREEN_START_ROW + 1; y <= SCREEN_END_ROW; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                VGA_MEMORY[(y - 1) * VGA_WIDTH + x] = VGA_MEMORY[y * VGA_WIDTH + x];
            }
        }
        
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[SCREEN_END_ROW * VGA_WIDTH + x] = blank;
        }
        
        cursor_y = SCREEN_END_ROW;
    }
}

void vga_putc(char c) {
    if (c == '\n') {
        write_serial('\r');
    }
    write_serial(c);

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
        }
    } else {
        unsigned short attribute = text_color << 8;
        unsigned short* location = (unsigned short*)VGA_MEMORY + (cursor_y * VGA_WIDTH + cursor_x);
        *location = c | attribute;
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    scroll();
    update_cursor();
}

void vga_set_color(unsigned char fg, unsigned char bg) {
    text_color = fg | (bg << 4);
}

unsigned char vga_get_color(void) {
    return text_color;
}

void vga_set_cursor(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH) {
        cursor_x = x;
    }
    if (y >= SCREEN_START_ROW && y <= SCREEN_END_ROW) {
        cursor_y = y;
    }
    update_cursor();
}

void vga_puts(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        vga_putc(str[i]);
    }
}

void vga_put_hex(unsigned int n) {
    char hex_chars[] = "0123456789ABCDEF";
    vga_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        vga_putc(hex_chars[(n >> i) & 0xF]);
    }
}

void vga_put_dec(unsigned int n) {
    if (n == 0) {
        vga_putc('0');
        return;
    }
    char buf[32];
    int i = 0;
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        vga_putc(buf[j]);
    }
}

void vga_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%') {
            i++;
            switch (format[i]) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    vga_putc(c);
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    vga_puts(s);
                    break;
                }
                case 'x': {
                    unsigned int x = va_arg(args, unsigned int);
                    vga_put_hex(x);
                    break;
                }
                case 'd': {
                    int d = va_arg(args, int);
                    if (d < 0) {
                        vga_putc('-');
                        d = -d;
                    }
                    vga_put_dec((unsigned int)d);
                    break;
                }
                case 'u': {
                    unsigned int u = va_arg(args, unsigned int);
                    vga_put_dec(u);
                    break;
                }
                case '%': {
                    vga_putc('%');
                    break;
                }
                default: {
                    vga_putc('%');
                    vga_putc(format[i]);
                    break;
                }
            }
        } else {
            vga_putc(format[i]);
        }
    }
    
    va_end(args);
}
