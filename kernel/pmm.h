#ifndef PMM_H
#define PMM_H

#include <stddef.h>

#define PAGE_SIZE 4096

// Multiboot 1 structures
struct multiboot_mmap_entry {
    unsigned int size;
    unsigned int addr_low;
    unsigned int addr_high;
    unsigned int len_low;
    unsigned int len_high;
    unsigned int type;
} __attribute__((packed));

typedef struct multiboot_mmap_entry multiboot_memory_map_t;

struct multiboot_info {
    unsigned int flags;
    unsigned int mem_lower;
    unsigned int mem_upper;
    unsigned int boot_device;
    unsigned int cmdline;
    unsigned int mods_count;
    unsigned int mods_addr;
    unsigned int syms[4];
    unsigned int mmap_length;
    unsigned int mmap_addr;
    unsigned int drives_length;
    unsigned int drives_addr;
    unsigned int config_table;
    unsigned int boot_loader_name;
    unsigned int apm_table;
    unsigned int vbe_control_info;
    unsigned int vbe_mode_info;
    unsigned short vbe_mode;
    unsigned short vbe_interface_seg;
    unsigned short vbe_interface_off;
    unsigned short vbe_interface_len;
} __attribute__((packed));

typedef struct multiboot_info multiboot_info_t;

// PMM API
void pmm_init(multiboot_info_t* mbi);
void* pmm_alloc_page(void);
void pmm_free_page(void* page);

// Get total memory size in bytes
size_t pmm_get_total_memory(void);
size_t pmm_get_free_memory(void);

#endif
