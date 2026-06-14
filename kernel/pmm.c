#include "pmm.h"
#include "vga.h"

// Bitmap size: 1048576 pages / 32 bits = 32768 words (128 KB)
#define MAX_PAGES 1048576
static unsigned int pmm_bitmap[MAX_PAGES / 32];

// Trackers
static size_t total_memory_bytes = 0;
static size_t free_memory_bytes = 0;

extern unsigned int _kernel_end;

static inline void bitmap_set(unsigned int bit) {
    pmm_bitmap[bit / 32] |= (1 << (bit % 32));
}

static inline void bitmap_clear(unsigned int bit) {
    pmm_bitmap[bit / 32] &= ~(1 << (bit % 32));
}

static inline int bitmap_test(unsigned int bit) {
    return (pmm_bitmap[bit / 32] & (1 << (bit % 32))) != 0;
}

void pmm_init(multiboot_info_t* mbi) {
    // 1. Mark all memory as used (1) initially
    for (int i = 0; i < MAX_PAGES / 32; i++) {
        pmm_bitmap[i] = 0xFFFFFFFF;
    }

    // 2. Parse memory map if provided by multiboot
    if (mbi->flags & (1 << 6)) {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mbi->mmap_addr;
        unsigned int mmap_end = mbi->mmap_addr + mbi->mmap_length;

        while ((unsigned int)mmap < mmap_end) {
            // Check if available RAM (type == 1)
            if (mmap->type == 1) {
                unsigned long long start_addr = ((unsigned long long)mmap->addr_high << 32) | mmap->addr_low;
                unsigned long long length = ((unsigned long long)mmap->len_high << 32) | mmap->len_low;
                unsigned long long end_addr = start_addr + length;

                // Update total memory count
                if (end_addr > total_memory_bytes) {
                    total_memory_bytes = (size_t)end_addr;
                }

                // Convert address to page indices
                unsigned int start_page = (unsigned int)(start_addr / PAGE_SIZE);
                unsigned int end_page = (unsigned int)(end_addr / PAGE_SIZE);

                // Free available pages in the bitmap
                for (unsigned int page = start_page; page < end_page; page++) {
                    if (page < MAX_PAGES) {
                        bitmap_clear(page);
                        free_memory_bytes += PAGE_SIZE;
                    }
                }
            }
            // Move to next entry
            mmap = (multiboot_memory_map_t*)((unsigned int)mmap + mmap->size + sizeof(mmap->size));
        }
    } else {
        // Fallback if no memory map: assume 64MB of RAM starting at 0
        total_memory_bytes = 64 * 1024 * 1024;
        unsigned int end_page = total_memory_bytes / PAGE_SIZE;
        for (unsigned int page = 0; page < end_page; page++) {
            bitmap_clear(page);
            free_memory_bytes += PAGE_SIZE;
        }
    }

    // 3. Reserve first 1MB of memory (page 0 to 255) for BIOS/GRUB/VGA
    for (unsigned int page = 0; page < 256; page++) {
        if (!bitmap_test(page)) {
            bitmap_set(page);
            free_memory_bytes -= PAGE_SIZE;
        }
    }

    // 4. Reserve kernel memory (from 1MB up to kernel end)
    unsigned int kernel_start_page = 0x100000 / PAGE_SIZE; // 256
    unsigned int kernel_end_page = (((unsigned int)&_kernel_end) + PAGE_SIZE - 1) / PAGE_SIZE;

    for (unsigned int page = kernel_start_page; page < kernel_end_page; page++) {
        if (!bitmap_test(page)) {
            bitmap_set(page);
            free_memory_bytes -= PAGE_SIZE;
        }
    }

    vga_printf("PMM: Total Memory: %d MB, Free Memory: %d MB\n", 
               (int)(total_memory_bytes / (1024 * 1024)),
               (int)(free_memory_bytes / (1024 * 1024)));
}

void* pmm_alloc_page(void) {
    // Find the first free bit (0)
    for (unsigned int i = 0; i < MAX_PAGES / 32; i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFF) {
            // There is a free page in this word
            for (int bit = 0; bit < 32; bit++) {
                unsigned int page = i * 32 + bit;
                if (!bitmap_test(page)) {
                    bitmap_set(page);
                    free_memory_bytes -= PAGE_SIZE;
                    return (void*)(page * PAGE_SIZE);
                }
            }
        }
    }
    vga_printf("PMM: OUT OF PHYSICAL MEMORY!\n");
    return NULL; // Out of memory
}

void pmm_free_page(void* page) {
    unsigned int page_addr = (unsigned int)page;
    unsigned int page_idx = page_addr / PAGE_SIZE;

    if (page_idx < MAX_PAGES) {
        if (bitmap_test(page_idx)) {
            bitmap_clear(page_idx);
            free_memory_bytes += PAGE_SIZE;
        }
    }
}

size_t pmm_get_total_memory(void) {
    return total_memory_bytes;
}

size_t pmm_get_free_memory(void) {
    return free_memory_bytes;
}
