#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "mouse.h"
#include "thread.h"
#include "process.h"
#include "string.h"
#include "pmm.h"
#include "heap.h"

// External references to our user-mode test binaries
extern void* user_proc1_start;
extern unsigned int user_proc1_size;
extern void* user_proc2_start;
extern unsigned int user_proc2_size;

// ---- Thread demo functions ----

static volatile int demo_threads_running = 0;

static void demo_thread_A(void) {
    for (int i = 0; i < 80; i++) {
        vga_putc('A');
        for (volatile int j = 0; j < 500000; j++);
    }
    demo_threads_running--;
    thread_exit();
}

static void demo_thread_B(void) {
    for (int i = 0; i < 80; i++) {
        vga_putc('B');
        for (volatile int j = 0; j < 500000; j++);
    }
    demo_threads_running--;
    thread_exit();
}

static void demo_thread_C(void) {
    for (int i = 0; i < 80; i++) {
        vga_putc('C');
        for (volatile int j = 0; j < 500000; j++);
    }
    demo_threads_running--;
    thread_exit();
}

// ---- Shell implementation ----

#define CMD_BUF_SIZE 128

static void shell_print_prompt(void) {
    vga_printf("myos> ");
}

static void shell_print_help(void) {
    vga_printf("Available commands:\n");
    vga_printf("  help     - Show this help\n");
    vga_printf("  clear    - Clear the screen\n");
    vga_printf("  info     - Show system information\n");
    vga_printf("  threads  - Demo: spawn 3 kernel threads (A, B, C)\n");
    vga_printf("  procs    - Demo: spawn 2 isolated user-mode processes\n");
    vga_printf("  trace    - Toggle scheduler trace logging\n");
    vga_printf("  mouse    - Show current mouse position\n");
    vga_printf("  heap     - Show heap block dump\n");
}

static void shell_print_info(void) {
    vga_printf("=== System Information ===\n");
    vga_printf("Kernel: myos (BSIT338OS)\n");
    vga_printf("Architecture: i686 (x86 Protected Mode)\n");
    vga_printf("Total Memory: %u MB\n", (unsigned int)(pmm_get_total_memory() / (1024 * 1024)));
    vga_printf("Free Memory:  %u MB\n", (unsigned int)(pmm_get_free_memory() / (1024 * 1024)));
    vga_printf("Scheduler: Round Robin (quantum = 10 ticks, 100 Hz PIT)\n");
    vga_printf("==========================\n");
}

static void shell_cmd_threads(void) {
    vga_printf("Spawning 3 kernel threads (A, B, C) with interleaving...\n");
    demo_threads_running = 3;
    thread_create(demo_thread_A, 4096);
    thread_create(demo_thread_B, 4096);
    thread_create(demo_thread_C, 4096);

    // Wait for demo threads to finish
    while (demo_threads_running > 0) {
        __asm__ volatile("hlt");
    }
    vga_printf("\nThread demo complete.\n");
}

static void shell_cmd_procs(void) {
    vga_printf("Spawning 2 user-mode processes with isolated address spaces...\n");
    vga_printf("Process 1 writes 0x11111111, Process 2 writes 0x22222222 to same virtual addr.\n");
    vga_printf("'S'/'s' = memory isolation verified, 'F' = FAILED.\n");
    process_create(&user_proc1_start, user_proc1_size);
    process_create(&user_proc2_start, user_proc2_size);

    // Let them run for a bit
    for (volatile int i = 0; i < 5000000; i++);
    vga_printf("\nProcess demo running (processes continue in background).\n");
}

static void shell_cmd_mouse(void) {
    int x, y;
    mouse_get_position(&x, &y);
    int buttons = mouse_get_buttons();
    vga_printf("Mouse: X=%d, Y=%d, Left=%d, Right=%d\n",
               x, y, buttons & 1, (buttons >> 1) & 1);
}

void shell_run(void) {
    char cmd_buf[CMD_BUF_SIZE];
    int cmd_pos = 0;

    vga_printf("\n");
    vga_printf("========================================\n");
    vga_printf("  Welcome to myos Interactive Shell\n");
    vga_printf("  Type 'help' for available commands\n");
    vga_printf("========================================\n\n");

    shell_print_prompt();

    while (1) {
        if (!keyboard_has_char()) {
            __asm__ volatile("hlt");
            continue;
        }

        char c = keyboard_getchar();

        if (c == '\n') {
            vga_putc('\n');
            cmd_buf[cmd_pos] = '\0';

            if (cmd_pos > 0) {
                if (strcmp(cmd_buf, "help") == 0) {
                    shell_print_help();
                } else if (strcmp(cmd_buf, "clear") == 0) {
                    vga_clear();
                } else if (strcmp(cmd_buf, "info") == 0) {
                    shell_print_info();
                } else if (strcmp(cmd_buf, "threads") == 0) {
                    shell_cmd_threads();
                } else if (strcmp(cmd_buf, "procs") == 0) {
                    shell_cmd_procs();
                } else if (strcmp(cmd_buf, "trace") == 0) {
                    scheduling_trace = !scheduling_trace;
                    vga_printf("Scheduler trace: %s\n",
                               scheduling_trace ? "ON" : "OFF");
                } else if (strcmp(cmd_buf, "mouse") == 0) {
                    shell_cmd_mouse();
                } else if (strcmp(cmd_buf, "heap") == 0) {
                    heap_dump();
                } else {
                    vga_printf("Unknown command: '%s'. Type 'help'.\n", cmd_buf);
                }
            }

            cmd_pos = 0;
            shell_print_prompt();
        } else if (c == '\b') {
            if (cmd_pos > 0) {
                cmd_pos--;
                vga_putc('\b');
                vga_putc(' ');
                vga_putc('\b');
            }
        } else if (c == '\t') {
            // Ignore tabs
        } else {
            if (cmd_pos < CMD_BUF_SIZE - 1) {
                cmd_buf[cmd_pos++] = c;
                vga_putc(c);
            }
        }
    }
}
