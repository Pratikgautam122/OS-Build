#ifndef PAGING_H
#define PAGING_H

#include <stddef.h>

#define PAGE_PRESENT  0x01
#define PAGE_WRITE    0x02
#define PAGE_USER     0x04

// Represents a page directory (1024 entries)
typedef unsigned int page_directory_t[1024];

// Represents a page table (1024 entries)
typedef unsigned int page_table_t[1024];

void paging_init(void);
void paging_switch_directory(page_directory_t* dir);
void paging_map(page_directory_t* dir, unsigned int virt, unsigned int phys, int flags);
void paging_unmap(page_directory_t* dir, unsigned int virt);
unsigned int paging_get_phys(page_directory_t* dir, unsigned int virt);

page_directory_t* paging_get_kernel_directory(void);

#endif
