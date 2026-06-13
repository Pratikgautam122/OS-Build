; kernel/interrupts.asm
extern interrupt_handler

%macro ISR_NOERRCODE 1
  global isr%1
  isr%1:
    push byte 0   ; Push dummy error code
    push %1       ; Push interrupt number
    jmp interrupt_common_stub
%endmacro

%macro ISR_ERRCODE 1
  global isr%1
  isr%1:
    ; CPU already pushed the error code
    push %1       ; Push interrupt number
    jmp interrupt_common_stub
%endmacro

%macro IRQ 2
  global irq%1
  irq%1:
    push byte 0   ; Push dummy error code
    push %2       ; Push interrupt number (32 + %1)
    jmp interrupt_common_stub
%endmacro

; Define all 32 CPU exceptions
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31

; Define IRQs remapped to 32-47
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; Syscall handler
global isr128
isr128:
    push byte 0
    push 128
    jmp interrupt_common_stub

; Common interrupt handler stub
interrupt_common_stub:
    pusha             ; Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax

    mov ax, ds        ; Get data segment selector
    push eax          ; Push ds selector onto stack

    mov ax, 0x10      ; Load kernel data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp          ; Pass pointer to registers_t on stack
    call interrupt_handler
    add esp, 4        ; Clean up esp parameter

    pop eax           ; Restore original data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa              ; Pop edi, esi, ebp, etc.
    add esp, 8        ; Clean up error code and interrupt number
    iret              ; Return from interrupt (enables interrupts if they were enabled before)

global context_switch
; void context_switch(thread_t* old, thread_t* new)
context_switch:
    push ebp
    push ebx
    push esi
    push edi

    mov eax, [esp + 20]     ; Get old TCB pointer
    mov [eax], esp          ; Save current ESP in old->esp

    mov edx, [esp + 24]     ; Get new TCB pointer
    mov esp, [edx]          ; Load new ESP from new->esp

    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

global enter_usermode
; void enter_usermode(unsigned int entry_fn, unsigned int user_stack)
enter_usermode:
    cli
    mov ecx, [esp + 4]   ; ecx = entry_fn
    mov edx, [esp + 8]   ; edx = user_stack

    mov ax, 0x23         ; User data segment selector (0x20 | 3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push 0x23            ; User SS
    push edx             ; User ESP

    pushfd               ; Push EFLAGS
    pop eax
    or eax, 0x200        ; Set Interrupt Flag (IF)
    push eax             ; EFLAGS

    push 0x1B            ; User CS
    push ecx             ; User EIP

    iret                 ; Switch to Ring 3 (user mode)
