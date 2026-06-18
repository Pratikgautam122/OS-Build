# BSIT338OS (myos) - 32-bit x86 Protected Mode Kernel

A lightweight, Multiboot-1 compliant x86 operating system written in C and Assembly, running in 32-bit Protected Mode. The system features a custom memory management stack, cooperative and preemptive multi-threaded scheduling, virtual address space isolation for user-mode processes, basic device drivers, system calls, and an interactive command shell.

---

## 🚀 Key Features

| Component | Description |
|---|---|
| **Bootloader** | Multiboot-1 compliant boot setup, loaded via GRUB to produce a bootable ISO (`myos.iso`). |
| **GDT & TSS** | Configures GDT segments for both kernel (Ring 0) and user space (Ring 3), incorporating TSS for Ring transitions. |
| **Interrupts (IDT & PIC)** | Handles CPU exceptions and remapped hardware interrupts (IRQ0-15), supporting keyboard, mouse, and timer IRQs. |
| **Physical Memory Manager** | Bitmap-based PMM that parses the Multiboot memory map structure to dynamically manage available RAM. |
| **Virtual Memory Manager** | Directory-based two-level Paging system, identity-mapping the kernel and dynamically mapping heap/process regions. |
| **Dynamic Heap** | Custom allocator supporting [kmalloc](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/heap.c#L84) and [kfree](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/heap.c#L119) with block splitting, coalescing, and dynamic physical memory expansion. |
| **PS/2 Device Drivers** | Keyboard translation (scancode mapping, buffer queue, backspace handling) and Mouse tracking (decoding raw PS/2 packets). |
| **Thread Scheduler** | Round-robin preemptive scheduler running at 100 Hz, with support for cooperative yielding and thread state transitions. |
| **Process Isolation** | Spawns user-mode processes with isolated page directories (CR3 switching) verifying protection of local address spaces. |
| **System Calls** | Software interrupt interface (`int 0x80`) enabling safe execution of kernel services from Ring 3 processes. |
| **Interactive Shell** | Full CLI interface on the VGA screen with tools to inspect heap blocks, toggle scheduler trace, and verify features. |

---

## 📁 Repository Structure

* [Makefile](file:///Users/pratikgautam/Documents/BSIT338OS/Makefile) - Automated build and emulation targets.
* [linker.ld](file:///Users/pratikgautam/Documents/BSIT338OS/linker.ld) - Kernel linker script (sections aligned to 4KB, starting at 1MB).
* [boot.asm](file:///Users/pratikgautam/Documents/BSIT338OS/boot.asm) - Assembly entrypoint, Multiboot headers, stack setup.
* **kernel/** - Kernel Source Code:
  * [kernel.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/kernel.c) - Kernel main initialization flow ([kmain](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/kernel.c#L14)).
  * [gdt.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/gdt.c) / [gdt.h](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/gdt.h) / [gdt_flush.asm](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/gdt_flush.asm) - Global Descriptor Table and Task State Segment.
  * [idt.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/idt.c) / [idt.h](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/idt.h) / [interrupts.asm](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/interrupts.asm) - Interrupt Descriptor Table & ISR wrappers.
  * [pic.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/pic.c) / [pic.h](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/pic.h) - PIC remapping and mask utilities.
  * [io.h](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/io.h) - Low-level I/O port instructions (`inb`, `outb`).
  * [vga.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/vga.c) / [vga.h](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/vga.h) - VGA text mode driver & custom `vga_printf`.
  * [string.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/string.c) / [string.h](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/string.h) - Basic string functions (strcmp, strlen, etc.).
  * [pmm.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/pmm.c) / [pmm.h](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/pmm.h) - Bitmap physical page frame allocator ([pmm_init](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/pmm.c#L26)).
  * [paging.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/paging.c) / [paging.h](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/paging.h) - Page directory/table setup & mapping functions.
  * [heap.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/heap.c) / [heap.h](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/heap.h) - Kernel block-allocator with [heap_grow](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/heap.c#L40).
  * [keyboard.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/keyboard.c) / [keyboard.h](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/keyboard.h) - PS/2 Keyboard IRQ handler.
  * [mouse.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/mouse.c) / [mouse.h](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/mouse.h) - PS/2 Mouse IRQ handler & position tracker.
  * [thread.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/thread.c) / [thread.h](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/thread.h) - Threads, scheduler, PIT programming.
  * [process.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/process.c) / [process.h](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/process.h) - Process control structures and creation.
  * [syscall.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/syscall.c) / [syscall.h](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/syscall.h) - Syscall handler registration ([syscall_init](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/syscall.c#L39)).
  * [user_binaries.asm](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/user_binaries.asm) - Test user processes written in Assembly.

---

## 🛠️ Requirements & Toolchain

To build and run this operating system, you will need:

1. **Assembler**: [NASM](https://www.nasm.us/)
2. **Compiler & Linker**: An x86 ELF cross-compiler toolchain targeting `i686-elf` (specifically `i686-elf-gcc`, `i686-elf-ld`).
3. **ISO Builder**: `grub-mkrescue` (part of the GRUB suite) and `xorriso`.
4. **Emulator**: [QEMU](https://www.qemu.org/) (specifically `qemu-system-i386`).

### Installation on macOS
```bash
brew install nasm qemu xorriso
# Note: You must have i686-elf-gcc installed and in your PATH to compile the sources.
```

---

## ⚙️ Compilation & Execution

The system uses a `Makefile` to automate compilation and running commands.

### 1. Build the OS
Compile source files, link the kernel, and build a bootable ISO:
```bash
make
```
This builds `kernel.bin` and generates the bootable image `myos.iso` under the root directory.

### 2. Run in QEMU
Launch the emulator with the generated ISO, mapping the serial interface to standard output:
```bash
make run
```

### 3. Clean Build Artifacts
Clean object files, the kernel binary, and the generated ISO:
```bash
make clean
```

---

## 🔍 Detailed Component Walkthrough

### 1. Boot Sequence & Multiboot Headers
When the GRUB bootloader loads `myos.iso`, it searches for the Multiboot-1 compliant header defined in [boot.asm](file:///Users/pratikgautam/Documents/BSIT338OS/boot.asm). GRUB then sets up the CPU, loads the kernel at physical address `1MB` (as instructed by [linker.ld](file:///Users/pratikgautam/Documents/BSIT338OS/linker.ld)), passes the Multiboot magic signature (`0x2BADB002`) and the info structure address in registers `eax` and `ebx`, and jumps to `_start`.

The `_start` entry point:
- Sets up the temporary 16 KiB kernel stack (`stack_top`).
- Pushes the Multiboot magic number and info address pointer.
- Jumps to the kernel entrypoint function `kmain` in [kernel.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/kernel.c).

### 2. Initialization Phases (`kmain`)
Upon entering `kmain`, the kernel initializes subsystems sequentially:
1. **VGA Console**: Sets up standard 80x25 text mode terminal outputs.
2. **GDT & TSS**: Populates descriptor tables and loads TSS.
3. **IDT & PIC**: Remaps the PIC offsets to `0x20` and `0x28` to separate hardware interrupts from CPU exceptions.
4. **PMM**: Initializes the bitmap physical memory manager based on the GRUB memory map.
5. **Paging**: Enables virtual memory mapping and identity-maps the lower 16MB.
6. **Heap**: Sets up a 1MB virtual heap starting at `0x10000000` mapped onto allocated physical pages.
7. **Drivers**: Configures IRQ1 (Keyboard) and IRQ12 (PS/2 Mouse) interrupt routines.
8. **Scheduling & Subsystems**: Prepares thread control blocks, registers system calls (`int 0x80`), programs the PIT to 100 Hz, enables CPU interrupts (`sti`), and spins up the Shell.

### 3. Memory Architecture
* **Physical Memory Manager**: Located in [pmm.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/pmm.c), it maintains a page bitmap where each bit represents a 4KB block of RAM. It automatically protects BIOS, video RAM, and the loaded kernel code space.
* **Virtual Memory (Paging)**: Set up in [paging.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/paging.c). It clones the page directory for new user processes so they map their own stack/data pages while keeping the kernel mapped in upper memory regions.
* **Kernel Heap**: Described in [heap.c](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/heap.c). It uses a singly linked list of block headers tracking sizes, flags, and pointers. When allocations exceed current heap bounds, it triggers [heap_grow](file:///Users/pratikgautam/Documents/BSIT338OS/kernel/heap.c#L40) to request additional page frames from the PMM.

### 4. Processes & Address Space Isolation
Using the `procs` shell command, the OS launches two concurrent processes. Each process gets its own physical page directory:
* Virtual address space isolation is demonstrated by having both processes execute at identical virtual addresses (specifically, modifying memory at virtual address `0x40100F00`).
* Because paging registers are updated during context switches, each process writes to its own isolated page frame. The shell logs `S` (success) and `s` when memory checks verify that each process's value was not corrupted by the other.

---

## 💻 Interactive Shell Commands

Once booted, the shell is exposed on VGA and standard output. The available commands are:

| Command | Action |
|---|---|
| `help` | Lists all interactive shell commands. |
| `clear` | Clears the VGA text terminal screen. |
| `info` | Prints general system info, architectural specs, and PMM memory statistics. |
| `threads` | Spawns 3 kernel threads that interleave execution on the scheduler. |
| `procs` | Spawns 2 isolated user-mode processes to demonstrate address space virtual memory mapping checks. |
| `trace` | Toggles scheduler trace log output to show active context switching. |
| `mouse` | Prints current X/Y cursor offsets and button flags from the mouse packet queue. |
| `heap` | Performs a block dump of the kernel heap, displaying start addresses, block sizes, and status. |
