#include "keyboard.h"
#include "idt.h"
#include "pic.h"
#include "io.h"
#include "vga.h"

#define KB_BUFFER_SIZE 256
static char kb_buffer[KB_BUFFER_SIZE];
static int kb_head = 0;
static int kb_tail = 0;

static int shift_pressed = 0;
static int ctrl_pressed = 0;

// Scan code set 1 lookup table
static const char kbdus[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* 42   - Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.',	/* 52 */
  '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

// Shifted QWERTY lookup table
static const char kbdus_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*',	/* 9 */
  '(', ')', '_', '+', '\b',	/* Backspace */
  '\t',			/* Tab */
  'Q', 'W', 'E', 'R',	/* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	/* 39 */
 '"', '~',   0,		/* 42   - Left shift */
 '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>',	/* 52 */
  '?',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

static void kb_push(char c) {
    int next = (kb_head + 1) % KB_BUFFER_SIZE;
    if (next != kb_tail) {
        kb_buffer[kb_head] = c;
        kb_head = next;
    }
}

static char kb_pop(void) {
    if (kb_head == kb_tail) {
        return 0;
    }
    char c = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
    return c;
}

int keyboard_has_char(void) {
    return kb_head != kb_tail;
}

// Forward declaration of waking thread helper (we'll implement this in the scheduler later)
extern void scheduler_wake_keyboard_waiters(void);

static void keyboard_interrupt_handler(registers_t* regs) {
    (void)regs;
    unsigned char scancode = inb(0x60);

    // If key released (bit 7 set)
    if (scancode & 0x80) {
        unsigned char press_code = scancode & ~0x80;
        if (press_code == 0x2A || press_code == 0x36) {
            shift_pressed = 0;
        }
        if (press_code == 0x1D) {
            ctrl_pressed = 0;
        }
    } else {
        // Key pressed
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 1;
        } else if (scancode == 0x1D) {
            ctrl_pressed = 1;
        } else {
            char ascii = 0;
            if (scancode < 128) {
                ascii = shift_pressed ? kbdus_shift[scancode] : kbdus[scancode];
            }
            if (ascii != 0) {
                kb_push(ascii);
                // Wake any blocked threads (if scheduler is ready)
                scheduler_wake_keyboard_waiters();
            }
        }
    }
}

void keyboard_init(void) {
    // 1. Register interrupt handler (IRQ 1 is interrupt 33)
    register_interrupt_handler(33, keyboard_interrupt_handler);

    // 2. Unmask IRQ 1 on the master PIC
    irq_clear_mask(1);
    
    vga_printf("Keyboard: Initialized driver (IRQ 1 unmasked).\n");
}

char keyboard_getchar(void) {
    // Busy loop if scheduler is not running yet
    while (!keyboard_has_char()) {
        __asm__ volatile("hlt"); // halt until interrupt occurs to save power
    }
    return kb_pop();
}
