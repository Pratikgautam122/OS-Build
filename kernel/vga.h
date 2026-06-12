#ifndef VGA_H
#define VGA_H

void vga_init(void);
void vga_clear(void);
void vga_putc(char c);
void vga_puts(const char* str);
void vga_put_hex(unsigned int n);
void vga_put_dec(unsigned int n);
void vga_printf(const char* format, ...);

#endif
