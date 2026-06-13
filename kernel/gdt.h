#ifndef GDT_H
#define GDT_H

// GDT entry structure
struct gdt_entry_struct {
    unsigned short limit_low;           // The lower 16 bits of the limit
    unsigned short base_low;            // The lower 16 bits of the base
    unsigned char  base_middle;         // The next 8 bits of the base
    unsigned char  access;              // Access flags, determine what ring this segment can be used in
    unsigned char  granularity;
    unsigned char  base_high;           // The last 8 bits of the base
} __attribute__((packed));

typedef struct gdt_entry_struct gdt_entry_t;

// GDT pointer structure
struct gdt_ptr_struct {
    unsigned short limit;               // The upper 16 bits of all selector limits
    unsigned int   base;                // The address of the first gdt_entry_t struct
} __attribute__((packed));

typedef struct gdt_ptr_struct gdt_ptr_t;

// TSS structure
struct tss_entry_struct {
    unsigned int link;
    unsigned int esp0;       // Stack pointer to load when entering ring 0
    unsigned int ss0;        // Stack segment to load when entering ring 0
    unsigned int esp1;
    unsigned int ss1;
    unsigned int esp2;
    unsigned int ss2;
    unsigned int cr3;
    unsigned int eip;
    unsigned int eflags;
    unsigned int eax;
    unsigned int ecx;
    unsigned int edx;
    unsigned int ebx;
    unsigned int esp;
    unsigned int ebp;
    unsigned int esi;
    unsigned int edi;
    unsigned int es;
    unsigned int cs;
    unsigned int ss;
    unsigned int ds;
    unsigned int fs;
    unsigned int gs;
    unsigned int ldt;
    unsigned short trap;
    unsigned short iomap_base;
} __attribute__((packed));

typedef struct tss_entry_struct tss_entry_t;

// Initializer
void gdt_init(void);

// TSS update helper
void update_tss_esp(unsigned int esp);

#endif
