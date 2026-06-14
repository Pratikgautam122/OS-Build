#include "paging.h"
#include "pmm.h"
#include "vga.h"
#include "string.h"

__attribute__((aligned(4096))) static page_directory_t kernel_directory;
__attribute__((aligned(4096))) static page_table_t kernel_tables[4]; // Identity map 16MB (4 tables * 1024 pages)

void paging_init(void) {
    // 1. Clear directory entries
    for (int i = 0; i < 1024; i++) {
        kernel_directory[i] = 0;
    }

    // 2. Clear initial page tables
    for (int t = 0; t < 4; t++) {
        for (int e = 0; e < 1024; e++) {
            kernel_tables[t][e] = 0;
        }
    }

    // 3. Identity map first 16MB (0x00000000 - 0x00FFFFFF)
    for (unsigned int i = 0; i < 4096; i++) {
        unsigned int phys = i * PAGE_SIZE;
        kernel_tables[i / 1024][i % 1024] = phys | PAGE_PRESENT | PAGE_WRITE;
    }

    // 4. Fill page directory with these tables
    for (int t = 0; t < 4; t++) {
        kernel_directory[t] = ((unsigned int)&kernel_tables[t]) | PAGE_PRESENT | PAGE_WRITE;
    }

    // 5. Recursive mapping: last directory entry points to the directory itself
    kernel_directory[1023] = ((unsigned int)&kernel_directory) | PAGE_PRESENT | PAGE_WRITE;

    // 6. Switch to kernel directory
    paging_switch_directory(&kernel_directory);

    // 7. Enable paging in CR0
    unsigned int cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Enable PG bit
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));

    vga_printf("Paging: Enabled paging (16MB identity mapped).\n");
}

void paging_switch_directory(page_directory_t* dir) {
    // Load CR3 with physical address of the directory
    __asm__ volatile("mov %0, %%cr3" : : "r"(dir));
}

page_directory_t* paging_get_kernel_directory(void) {
    return &kernel_directory;
}

void paging_map(page_directory_t* dir, unsigned int virt, unsigned int phys, int flags) {
    unsigned int pd_idx = virt >> 22;
    unsigned int pt_idx = (virt >> 12) & 0x3FF;

    // If dir is the current directory, we can use the recursive mapping
    // Otherwise, we write to the directory directly (if it is within the identity-mapped region)
    // or map it temporarily. In our design, all page directories are accessed while their physical page
    // is mapped or when active.
    unsigned int* pd = (unsigned int*)0xFFFFF000;
    
    // Check if the page directory entry is present
    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        // Allocate a new physical page for the page table
        void* pt_phys = pmm_alloc_page();
        if (!pt_phys) {
            vga_printf("Paging: Failed to allocate physical page for page table!\n");
            return;
        }

        // Set the PDE (User flag is inherited if flags specify it)
        pd[pd_idx] = ((unsigned int)pt_phys) | (flags & (PAGE_WRITE | PAGE_USER)) | PAGE_PRESENT;
        
        // Invalidate TLB for the new page table mapping
        unsigned int pt_virt = 0xFFC00000 + (pd_idx * 4096);
        __asm__ volatile("invlpg (%0)" : : "r"(pt_virt));

        // Clear the new page table
        memset((void*)pt_virt, 0, PAGE_SIZE);
    }

    // Now map the physical page in the page table
    unsigned int* pt = (unsigned int*)(0xFFC00000 + (pd_idx * 4096));
    pt[pt_idx] = (phys & ~0xFFF) | (flags & 0xFFF) | PAGE_PRESENT;

    // Invalidate TLB for the mapped virtual address
    __asm__ volatile("invlpg (%0)" : : "r"(virt));
}

void paging_unmap(page_directory_t* dir, unsigned int virt) {
    unsigned int pd_idx = virt >> 22;
    unsigned int pt_idx = (virt >> 12) & 0x3FF;

    unsigned int* pd = (unsigned int*)0xFFFFF000;
    if (pd[pd_idx] & PAGE_PRESENT) {
        unsigned int* pt = (unsigned int*)(0xFFC00000 + (pd_idx * 4096));
        pt[pt_idx] = 0;
        __asm__ volatile("invlpg (%0)" : : "r"(virt));
    }
}

unsigned int paging_get_phys(page_directory_t* dir, unsigned int virt) {
    unsigned int pd_idx = virt >> 22;
    unsigned int pt_idx = (virt >> 12) & 0x3FF;

    unsigned int* pd = (unsigned int*)0xFFFFF000;
    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        return 0;
    }

    unsigned int* pt = (unsigned int*)(0xFFC00000 + (pd_idx * 4096));
    if (!(pt[pt_idx] & PAGE_PRESENT)) {
        return 0;
    }

    return (pt[pt_idx] & ~0xFFF) + (virt & 0xFFF);
}
