#include "syscall.h"
#include "idt.h"
#include "thread.h"
#include "vga.h"
#include "string.h"

static void syscall_handler(registers_t* regs) {
    unsigned int syscall_no = regs->eax;

    switch (syscall_no) {
        case SYS_WRITE: {
            const char* str = (const char*)regs->ebx;
            unsigned char old_color = vga_get_color();
            if (strcmp(str, "1") == 0) {
                vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
            } else if (strcmp(str, "2") == 0) {
                vga_set_color(VGA_COLOR_BROWN, VGA_COLOR_BLACK);
            } else if (strcmp(str, "S") == 0 || strcmp(str, "s") == 0) {
                vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            } else if (strcmp(str, "F") == 0) {
                vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            }
            vga_printf("%s", str);
            vga_set_color(old_color & 0x0F, (old_color >> 4) & 0x0F);
            regs->eax = strlen(str);
            break;
        }
        case SYS_YIELD: {
            thread_yield();
            regs->eax = 0;
            break;
        }
        case SYS_EXIT: {
            vga_printf("Process exited via sys_exit.\n");
            thread_exit();
            break;
        }
        case SYS_SBRK: {
            // Basic stub
            regs->eax = 0;
            break;
        }
        default:
            vga_printf("Syscall: Unknown syscall %d\n", syscall_no);
            regs->eax = (unsigned int)-1;
            break;
    }
}

void syscall_init(void) {
    // Register interrupt 128 (0x80)
    register_interrupt_handler(128, syscall_handler);
    vga_printf("Syscall: Registered int 0x80 handler.\n");
}
