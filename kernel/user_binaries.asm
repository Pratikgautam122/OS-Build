; kernel/user_binaries.asm
global user_proc1_start
global user_proc1_size
global user_proc2_start
global user_proc2_size

section .rodata

user_proc1_start:
    ; Write 0x11111111 to 0x40100F00 (stack page is writable)
    mov eax, 0x40100F00
    mov dword [eax], 0x11111111
    
    ; Print '1'
    push 0
    push 0x31          ; ASCII '1'
    mov ebx, esp
    mov eax, 1         ; SYS_WRITE
    int 0x80
    add esp, 8
    
    ; Yield
    mov eax, 2         ; SYS_YIELD
    int 0x80
    
    ; Check if memory value was preserved
    mov eax, 0x40100F00
    mov ecx, [eax]
    cmp ecx, 0x11111111
    je .ok
    
    ; Fail: print 'F'
    push 0
    push 0x46          ; ASCII 'F'
    mov ebx, esp
    mov eax, 1         ; SYS_WRITE
    int 0x80
    add esp, 8
    jmp .end
.ok:
    ; Success: print 'S' (capital S)
    push 0
    push 0x53          ; ASCII 'S'
    mov ebx, esp
    mov eax, 1         ; SYS_WRITE
    int 0x80
    add esp, 8
.end:
    ; Yield again
    mov eax, 2         ; SYS_YIELD
    int 0x80
    jmp user_proc1_start
user_proc1_end:

user_proc1_size: dd (user_proc1_end - user_proc1_start)


user_proc2_start:
    ; Write 0x22222222 to 0x40100F00
    mov eax, 0x40100F00
    mov dword [eax], 0x22222222
    
    ; Print '2'
    push 0
    push 0x32          ; ASCII '2'
    mov ebx, esp
    mov eax, 1         ; SYS_WRITE
    int 0x80
    add esp, 8
    
    ; Yield
    mov eax, 2         ; SYS_YIELD
    int 0x80
    
    ; Check if memory value was preserved
    mov eax, 0x40100F00
    mov ecx, [eax]
    cmp ecx, 0x22222222
    je .ok
    
    ; Fail: print 'F'
    push 0
    push 0x46          ; ASCII 'F'
    mov ebx, esp
    mov eax, 1         ; SYS_WRITE
    int 0x80
    add esp, 8
    jmp .end
.ok:
    ; Success: print 's' (lowercase s)
    push 0
    push 0x73          ; ASCII 's'
    mov ebx, esp
    mov eax, 1         ; SYS_WRITE
    int 0x80
    add esp, 8
.end:
    ; Yield again
    mov eax, 2         ; SYS_YIELD
    int 0x80
    jmp user_proc2_start
user_proc2_end:

user_proc2_size: dd (user_proc2_end - user_proc2_start)
