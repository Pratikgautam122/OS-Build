; kernel/gdt_flush.asm
global gdt_flush
global tss_flush

gdt_flush:
    mov eax, [esp+4]  ; Get the pointer to the GDT descriptor (passed as parameter)
    lgdt [eax]        ; Load the new GDT

    mov ax, 0x10      ; 0x10 is the kernel data segment offset in GDT
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush   ; 0x08 is the kernel code segment offset (far jump)
.flush:
    ret

tss_flush:
    mov ax, 0x2B      ; 0x28 (TSS offset) | 0x03 (RPL 3) = 0x2B
    ltr ax            ; Load task register
    ret
