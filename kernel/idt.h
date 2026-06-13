#ifndef IDT_H
#define IDT_H

// IDT gate structure
struct idt_entry_struct {
    unsigned short base_lo;             // The lower 16 bits of the address to jump to
    unsigned short sel;                 // Kernel segment selector (usually 0x08)
    unsigned char  always0;             // This must always be zero
    unsigned char  flags;               // More flags (Type, DPL, Present)
    unsigned short base_hi;             // The upper 16 bits of the address to jump to
} __attribute__((packed));

typedef struct idt_entry_struct idt_entry_t;

// IDT pointer structure
struct idt_ptr_struct {
    unsigned short limit;
    unsigned int   base;                // The address of the first element in our idt_entry_t array
} __attribute__((packed));

typedef struct idt_ptr_struct idt_ptr_t;

// Register state passed to C handler during interrupts
struct registers_struct {
    unsigned int ds;                                     // Data segment selector
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    unsigned int int_no, err_code;                       // Interrupt number and error code
    unsigned int eip, cs, eflags, useresp, ss;           // Pushed by CPU automatically
};

typedef struct registers_struct registers_t;

typedef void (*interrupt_handler_t)(registers_t*);

void idt_init(void);
void register_interrupt_handler(unsigned char n, interrupt_handler_t handler);
void interrupt_handler(registers_t* regs);

#endif
