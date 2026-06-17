#include "process.h"
#include "heap.h"
#include "pmm.h"
#include "vga.h"
#include "string.h"

extern void enter_usermode(unsigned int entry_fn, unsigned int user_stack);

static int next_pid = 1;

static void process_thread_entry(void) {
    thread_t* curr = scheduler_get_current_thread();
    process_t* proc = curr->process;

    // 1. Switch to process's private page directory
    paging_switch_directory(proc->page_directory);

    // 2. Drop to User Mode (Ring 3)
    // Code is mapped at 0x40000000, stack is mapped at 0x40100000 (grows down from 0x40101000)
    enter_usermode(0x40000000, 0x40100000 + 4096);
}

void process_init(void) {
    vga_printf("Process: Subsystem initialized.\n");
}

process_t* process_create(void* code_src, unsigned int code_size) {
    __asm__ volatile("cli");

    // 1. Allocate page directory
    page_directory_t* new_dir_phys = (page_directory_t*)pmm_alloc_page();
    if (!new_dir_phys) {
        vga_printf("Process: Failed to allocate page directory!\n");
        __asm__ volatile("sti");
        return NULL;
    }

    // 2. Clone kernel page directory (first 4 entries for 16MB)
    // We map it temporarily in our current space to write into it
    unsigned int current_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(current_cr3));
    
    // Switch to directory to map the directories (paging_map works recursively on CR3)
    // Wait, paging_map requires recursive mapping of the active page directory.
    // So to modify mappings, we can write to the directory directly if it's mapped.
    // Or we can just copy to the new directory by mapping it.
    // Since the new page directory page is not mapped, we can map it to virtual address 0x20000000
    // of the active page directory first.
    page_directory_t* active_pd = (page_directory_t*)current_cr3;
    paging_map(active_pd, 0x20000000, (unsigned int)new_dir_phys, PAGE_WRITE);
    
    unsigned int* new_dir_virt = (unsigned int*)0x20000000;
    memset(new_dir_virt, 0, PAGE_SIZE);

    unsigned int* kdir = (unsigned int*)paging_get_kernel_directory();
    for (int i = 0; i < 1023; i++) {
        new_dir_virt[i] = kdir[i];
    }
    // Recursive mapping
    new_dir_virt[1023] = ((unsigned int)new_dir_phys) | PAGE_PRESENT | PAGE_WRITE;

    paging_unmap(active_pd, 0x20000000);

    // 3. Allocate physical pages for user code and user stack
    void* code_phys = pmm_alloc_page();
    void* stack_phys = pmm_alloc_page();
    if (!code_phys || !stack_phys) {
        vga_printf("Process: Failed to allocate physical pages for user space!\n");
        __asm__ volatile("sti");
        return NULL;
    }

    // 4. Switch to new directory temporarily to map user pages and copy code
    paging_switch_directory(new_dir_phys);

    // Map code at 0x40000000 and stack at 0x40100000 with PAGE_USER flag
    paging_map(new_dir_phys, 0x40000000, (unsigned int)code_phys, PAGE_WRITE | PAGE_USER);
    paging_map(new_dir_phys, 0x40100000, (unsigned int)stack_phys, PAGE_WRITE | PAGE_USER);

    // Copy user code bytes
    memcpy((void*)0x40000000, code_src, code_size);

    // Restore old directory
    paging_switch_directory((page_directory_t*)current_cr3);

    // 5. Create PCB
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    proc->pid = next_pid++;
    proc->parent_pid = 0;
    proc->state = PROC_READY;
    proc->page_directory = new_dir_phys;

    // 6. Create main thread of process
    thread_t* t = thread_create(process_thread_entry, 8192);
    t->process = proc;
    proc->main_thread = t;

    __asm__ volatile("sti");
    return proc;
}
