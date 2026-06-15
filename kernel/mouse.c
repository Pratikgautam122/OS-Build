#include "mouse.h"
#include "idt.h"
#include "pic.h"
#include "io.h"
#include "vga.h"

static int mouse_x = 40;
static int mouse_y = 12;
static int old_mouse_x = 40;
static int old_mouse_y = 12;
static unsigned short old_char_data = ' ' | (0x07 << 8);

static int left_button = 0;
static int right_button = 0;

static unsigned char mouse_cycle = 0;
static unsigned char mouse_packet[3];

static void mouse_wait(unsigned char type) {
    unsigned int timeout = 100000;
    if (type == 0) {
        while ((inb(0x64) & 1) == 0 && timeout > 0) timeout--;
    } else {
        while ((inb(0x64) & 2) != 0 && timeout > 0) timeout--;
    }
}

static void mouse_write(unsigned char value) {
    mouse_wait(1);
    outb(0x64, 0xD4); // Tell 8042 to send byte to auxiliary device
    mouse_wait(1);
    outb(0x60, value);
}

static unsigned char mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_render_cursor(void) {
    volatile unsigned short* vga = (volatile unsigned short*)0xB8000;

    // 1. Restore old cell data
    vga[old_mouse_y * 80 + old_mouse_x] = old_char_data;

    // 2. Capture new cell data
    old_mouse_x = mouse_x;
    old_mouse_y = mouse_y;
    old_char_data = vga[mouse_y * 80 + mouse_x];

    // 3. Set highlighted cursor attribute (swap foreground and background color)
    unsigned char attr = (old_char_data >> 8) & 0xFF;
    unsigned char fg = attr & 0x0F;
    unsigned char bg = (attr & 0xF0) >> 4;
    unsigned char new_attr = (fg << 4) | bg;
    if (new_attr == 0 || new_attr == attr) {
        new_attr = 0x70; // Inverted video (black on light grey)
    }

    vga[mouse_y * 80 + mouse_x] = (old_char_data & 0x00FF) | (new_attr << 8);
}

static void mouse_interrupt_handler(registers_t* regs) {
    (void)regs;
    
    // Check if mouse data is actually ready
    unsigned char status = inb(0x64);
    if (!(status & 0x01)) {
        return; // No data available
    }
    if (!(status & 0x20)) {
        return; // Not mouse data (keyboard data instead)
    }

    unsigned char data = inb(0x60);

    if (mouse_cycle == 0) {
        if (data & 0x08) { // Bit 3 must be 1 for a valid first byte
            mouse_packet[0] = data;
            mouse_cycle = 1;
        }
    } else if (mouse_cycle == 1) {
        mouse_packet[1] = data;
        mouse_cycle = 2;
    } else if (mouse_cycle == 2) {
        mouse_packet[2] = data;
        mouse_cycle = 0;

        int x_sign = (mouse_packet[0] & 0x10) ? 1 : 0;
        int y_sign = (mouse_packet[0] & 0x20) ? 1 : 0;

        int dx = mouse_packet[1];
        int dy = mouse_packet[2];

        if (x_sign) {
            dx -= 256;
        }
        if (y_sign) {
            dy -= 256;
        }

        // Update coordinates
        mouse_x += dx;
        mouse_y -= dy; // Inverted relative to screen Y coordinates

        // Clamp to screen bounds (80 x 25)
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x >= 80) mouse_x = 79;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y >= 25) mouse_y = 24;

        left_button = (mouse_packet[0] & 0x01) ? 1 : 0;
        right_button = (mouse_packet[0] & 0x02) ? 1 : 0;

        mouse_render_cursor();
    }
}

void mouse_init(void) {
    unsigned char status;

    // Enable auxiliary device
    mouse_wait(1);
    outb(0x64, 0xA8);

    // Enable interrupts
    mouse_wait(1);
    outb(0x64, 0x20); // Get command byte
    status = mouse_read() | 2; // Enable auxiliary device interrupt (bit 1)
    
    mouse_wait(1);
    outb(0x64, 0x60); // Set command byte
    mouse_wait(1);
    outb(0x60, status);

    // Use default settings
    mouse_write(0xF6);
    mouse_read(); // ACK (0xFA)

    // Enable data reporting
    mouse_write(0xF4);
    mouse_read(); // ACK (0xFA)

    // Register handler for IRQ 12 (interrupt 44)
    register_interrupt_handler(44, mouse_interrupt_handler);

    // Unmask IRQ 12 (slave) and IRQ 2 (cascade line on master PIC)
    irq_clear_mask(2);
    irq_clear_mask(12);

    // Draw initial cursor
    mouse_render_cursor();

    vga_printf("Mouse: Initialized driver (IRQ 12 unmasked).\n");
}

void mouse_get_position(int* x, int* y) {
    if (x) *x = mouse_x;
    if (y) *y = mouse_y;
}

int mouse_get_buttons(void) {
    return (left_button) | (right_button << 1);
}
