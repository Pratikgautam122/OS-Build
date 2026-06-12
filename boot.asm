; boot.asm
; Multiboot 1 compliant boot sector

MBALIGN     equ  1 << 0            ; align loaded modules on page boundaries
MEMINFO     equ  1 << 1            ; provide memory map
FLAGS       equ  MBALIGN | MEMINFO  ; this is the Multiboot 'flag' field
MAGIC       equ  0x1BADB002        ; 'magic number' lets bootloader find the header
CHECKSUM    equ  -(MAGIC + FLAGS)   ; checksum of above, to prove we are multiboot

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KiB stack
stack_top:

section .text
global _start
extern kmain

_start:
    ; Set up the stack pointer
    mov esp, stack_top

    ; Push multiboot information pointer (ebx) and magic number (eax)
    push ebx
    push eax

    ; Call the kernel main
    call kmain

    ; Infinite loop if kmain returns
.hang:
    cli
    hlt
    jmp .hang
