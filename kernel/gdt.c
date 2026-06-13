#include "gdt.h"
#include "string.h"

gdt_entry_t gdt_entries[6];
gdt_ptr_t   gdt_ptr;
tss_entry_t tss_entry;

extern void gdt_flush(unsigned int);
extern void tss_flush(void);

static void gdt_set_gate(int num, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = ((base >> 16) & 0xFF);
    gdt_entries[num].base_high   = ((base >> 24) & 0xFF);

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity  = ((limit >> 16) & 0x0F);

    gdt_entries[num].granularity |= (gran & 0xF0);
    gdt_entries[num].access      = access;
}

void gdt_init(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base  = (unsigned int)&gdt_entries;

    // 0: Null descriptor
    gdt_set_gate(0, 0, 0, 0, 0);
    
    // 1: Kernel Code: Base 0, Limit 4GB, Access 0x9A, Granularity 0xCF (32-bit, page granularity)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    
    // 2: Kernel Data: Base 0, Limit 4GB, Access 0x92, Granularity 0xCF
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    // 3: User Code: Base 0, Limit 4GB, Access 0xFA (Ring 3), Granularity 0xCF
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    
    // 4: User Data: Base 0, Limit 4GB, Access 0xF2 (Ring 3), Granularity 0xCF
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    // 5: TSS: Base &tss_entry, Limit sizeof(tss_entry) - 1, Access 0xE9 (DPL 3, Available 32-bit TSS)
    unsigned int tss_base = (unsigned int)&tss_entry;
    unsigned int tss_limit = sizeof(tss_entry) - 1;
    
    memset(&tss_entry, 0, sizeof(tss_entry));
    tss_entry.ss0  = 0x10;  // Kernel data segment selector
    tss_entry.esp0 = 0;     // Dynamically set per thread/process on context switch

    gdt_set_gate(5, tss_base, tss_limit, 0xE9, 0x00);

    gdt_flush((unsigned int)&gdt_ptr);
    tss_flush();
}

void update_tss_esp(unsigned int esp) {
    tss_entry.esp0 = esp;
}
