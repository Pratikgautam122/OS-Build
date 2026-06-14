#include "heap.h"
#include "paging.h"
#include "pmm.h"
#include "vga.h"

#define HEAP_START      0x10000000
#define HEAP_INIT_SIZE  0x100000      // 1 MB

struct heap_block {
    size_t size;
    int free;
    struct heap_block* next;
} __attribute__((packed));

static unsigned int heap_end = HEAP_START + HEAP_INIT_SIZE;

void heap_init(void) {
    page_directory_t* kdir = paging_get_kernel_directory();

    // Map physical pages for initial heap size
    unsigned int pages = HEAP_INIT_SIZE / PAGE_SIZE;
    for (unsigned int i = 0; i < pages; i++) {
        void* phys = pmm_alloc_page();
        if (!phys) {
            vga_printf("Heap: Failed to allocate physical page for heap init!\n");
            for (;;);
        }
        paging_map(kdir, HEAP_START + i * PAGE_SIZE, (unsigned int)phys, PAGE_WRITE);
    }

    // Initialize first heap block
    struct heap_block* first = (struct heap_block*)HEAP_START;
    first->size = HEAP_INIT_SIZE - sizeof(struct heap_block);
    first->free = 1;
    first->next = NULL;

    vga_printf("Heap: Initialized (1MB at %x).\n", HEAP_START);
}

static int heap_grow(size_t increment_size) {
    unsigned int pages = (increment_size + PAGE_SIZE - 1) / PAGE_SIZE;
    unsigned int bytes_to_add = pages * PAGE_SIZE;

    page_directory_t* kdir = paging_get_kernel_directory();
    for (unsigned int i = 0; i < pages; i++) {
        void* phys = pmm_alloc_page();
        if (!phys) {
            vga_printf("Heap: Failed to allocate physical page during heap growth!\n");
            return 0; // Failure
        }
        paging_map(kdir, heap_end + i * PAGE_SIZE, (unsigned int)phys, PAGE_WRITE);
    }

    // Create a new heap block in the new space
    struct heap_block* new_block = (struct heap_block*)heap_end;
    new_block->size = bytes_to_add - sizeof(struct heap_block);
    new_block->free = 1;
    new_block->next = NULL;

    // Find the last block in the list
    struct heap_block* curr = (struct heap_block*)HEAP_START;
    while (curr->next != NULL) {
        curr = curr->next;
    }
    curr->next = new_block;
    
    heap_end += bytes_to_add;

    // Coalesce free blocks
    curr = (struct heap_block*)HEAP_START;
    while (curr != NULL) {
        if (curr->free && curr->next && curr->next->free) {
            curr->size += sizeof(struct heap_block) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }

    vga_printf("Heap: Expanded by %d KB, New End: %x\n", bytes_to_add / 1024, heap_end);
    return 1;
}

void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    // Align size to 8 bytes
    size = (size + 7) & ~7;

    struct heap_block* curr = (struct heap_block*)HEAP_START;
    while (curr != NULL) {
        if (curr->free && curr->size >= size) {
            // Can we split this block?
            if (curr->size > size + sizeof(struct heap_block) + 8) {
                struct heap_block* split = (struct heap_block*)((char*)curr + sizeof(struct heap_block) + size);
                split->size = curr->size - size - sizeof(struct heap_block);
                split->free = 1;
                split->next = curr->next;

                curr->size = size;
                curr->next = split;
            }
            curr->free = 0;
            return (void*)((char*)curr + sizeof(struct heap_block));
        }
        curr = curr->next;
    }

    // No block found, need to grow heap
    if (heap_grow(size + sizeof(struct heap_block))) {
        return kmalloc(size); // Retry allocation
    }

    return NULL; // Out of memory
}

void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    struct heap_block* block = (struct heap_block*)((char*)ptr - sizeof(struct heap_block));
    block->free = 1;

    // Coalesce free blocks
    struct heap_block* curr = (struct heap_block*)HEAP_START;
    while (curr != NULL) {
        if (curr->free && curr->next && curr->next->free) {
            curr->size += sizeof(struct heap_block) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

void heap_dump(void) {
    vga_printf("=== Heap Dump ===\n");
    struct heap_block* curr = (struct heap_block*)HEAP_START;
    int index = 0;
    while (curr != NULL) {
        vga_printf("Block %d: Addr %x, Size %d bytes, %s\n", 
                   index++, (unsigned int)curr, curr->size, curr->free ? "FREE" : "USED");
        curr = curr->next;
    }
    vga_printf("=================\n");
}
