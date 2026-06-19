#ifndef VGA_H
#define VGA_H

// VGA 16 colors
#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE          1
#define VGA_COLOR_GREEN         2
#define VGA_COLOR_CYAN          3
#define VGA_COLOR_RED           4
#define VGA_COLOR_MAGENTA       5
#define VGA_COLOR_BROWN         6
#define VGA_COLOR_LIGHT_GREY    7
#define VGA_COLOR_DARK_GREY     8
#define VGA_COLOR_LIGHT_BLUE    9
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_YELLOW        14
#define VGA_COLOR_WHITE         15

void vga_init(void);
void vga_clear(void);
void vga_putc(char c);
void vga_puts(const char* str);
void vga_put_hex(unsigned int n);
void vga_put_dec(unsigned int n);
void vga_printf(const char* format, ...);

void vga_set_color(unsigned char fg, unsigned char bg);
unsigned char vga_get_color(void);
void vga_set_cursor(int x, int y);

#endif
