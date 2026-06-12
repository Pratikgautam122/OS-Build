#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "pmm.h"
#include "paging.h"
#include "heap.h"
#include "keyboard.h"
#include "mouse.h"
#include "thread.h"
#include "process.h"
#include "syscall.h"
#include "shell.h"

void kmain(unsigned int magic, unsigned int addr) {
    // --- Phase 1: VGA output ---
    vga_init();
    vga_printf("Kernel Booted Successfully!\n");
    vga_printf("Multiboot Magic: %x, Info Addr: %x\n\n", magic, addr);

    // --- Phase 2: Kernel infrastructure ---
    gdt_init();
    vga_printf("[OK] GDT and TSS loaded\n");

    idt_init();
    vga_printf("[OK] IDT loaded, PIC remapped (0x20/0x28)\n");

    multiboot_info_t* mbi = (multiboot_info_t*)addr;
    pmm_init(mbi);

    paging_init();
    heap_init();

    // --- Phase 3 & 4: Keyboard and Mouse ---
    keyboard_init();
    mouse_init();

    // --- Phase 5 & 7: Multithreading & Scheduler ---
    scheduler_init();
    pit_init(100); // 100 Hz Round Robin

    // --- Phase 6: Process subsystem & Syscalls ---
    process_init();
    syscall_init();

    // --- Enable hardware interrupts ---
    __asm__ volatile("sti");
    vga_printf("[OK] Hardware interrupts enabled\n\n");

    // --- Phase 8: Integration shell ---
    shell_run();
}
