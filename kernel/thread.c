#include "thread.h"
#include "heap.h"
#include "vga.h"
#include "gdt.h"
#include "pic.h"
#include "idt.h"
#include "io.h"
#include "paging.h"
#include "process.h"

extern void context_switch(thread_t* old, thread_t* new);

static thread_t* current_thread = NULL;
static thread_t* thread_list = NULL;
static int next_tid = 1;

static void idle_thread_fn(void) {
    while (1) {
        __asm__ volatile("hlt");
    }
}

static void thread_entry_wrapper(void (*entry_fn)(void)) {
    __asm__ volatile("sti");
    entry_fn();
    thread_exit();
}

void scheduler_init(void) {
    // 1. Disable interrupts during initialization
    __asm__ volatile("cli");

    // 2. Create main kernel thread TCB representing the boot path
    thread_t* main_thread = (thread_t*)kmalloc(sizeof(thread_t));
    main_thread->esp = NULL; // will be set on context switch away
    main_thread->tid = 0;
    main_thread->state = THREAD_RUNNING;
    main_thread->stack_limit = NULL; // already running on boot stack
    main_thread->stack_size = 0;
    main_thread->next = NULL;
    main_thread->process = NULL;

    thread_list = main_thread;
    current_thread = main_thread;

    // 3. Create idle thread
    thread_create(idle_thread_fn, 4096);

    vga_printf("Scheduler: Preemptive multithreading initialized.\n");
}

thread_t* thread_create(void (*entry_fn)(void), unsigned int stack_size) {
    __asm__ volatile("cli");

    thread_t* t = (thread_t*)kmalloc(sizeof(thread_t));
    void* stack_mem = kmalloc(stack_size);

    t->tid = next_tid++;
    t->state = THREAD_READY;
    t->stack_limit = stack_mem;
    t->stack_size = stack_size;
    t->process = NULL;

    // Set up initial stack content
    unsigned int top_of_stack = (unsigned int)stack_mem + stack_size;
    unsigned int* stack = (unsigned int*)top_of_stack;

    // Push arguments, wrapper return address, callee-saved registers
    stack[-1] = (unsigned int)entry_fn;
    stack[-2] = 0; // dummy return address
    stack[-3] = (unsigned int)thread_entry_wrapper;
    stack[-4] = 0; // ebp
    stack[-5] = 0; // ebx
    stack[-6] = 0; // esi
    stack[-7] = 0; // edi

    t->esp = (void*)(stack - 7);

    // Append to thread list
    thread_t* curr = thread_list;
    while (curr->next != NULL) {
        curr = curr->next;
    }
    curr->next = t;
    t->next = NULL;

    __asm__ volatile("sti");
    return t;
}

static unsigned int thread_ticks = 0;
#define QUANTUM 10

int scheduling_trace = 0;

void schedule(void) {
    if (!current_thread) return;

    // Find the next ready thread (Round Robin)
    thread_t* next = current_thread->next;
    while (1) {
        if (!next) {
            next = thread_list; // wrap around
        }
        if (next->state == THREAD_READY) {
            break;
        }
        if (next == current_thread) {
            // No other ready threads
            if (current_thread->state == THREAD_RUNNING) {
                return; // Keep running the current thread
            }
            // Since the idle thread is always ready, this loop is guaranteed to terminate.
        }
        next = next->next;
    }

    if (next != current_thread) {
        thread_t* old = current_thread;
        if (old->state == THREAD_RUNNING) {
            old->state = THREAD_READY;
        }
        next->state = THREAD_RUNNING;
        current_thread = next;

        // Reset quantum ticks on context switch
        thread_ticks = 0;

        if (scheduling_trace) {
            vga_printf("[Sched] TID %d -> TID %d\n", old->tid, next->tid);
        }

        // Update TSS kernel stack pointer (if this thread belongs to a user process)
        if (current_thread->stack_limit != NULL) {
            update_tss_esp((unsigned int)current_thread->stack_limit + current_thread->stack_size);
        }

        // Switch to the appropriate page directory (CR3)
        if (current_thread->process != NULL) {
            paging_switch_directory(current_thread->process->page_directory);
        } else {
            paging_switch_directory(paging_get_kernel_directory());
        }

        // Perform assembly context switch
        context_switch(old, next);
    }
}

void thread_yield(void) {
    __asm__ volatile("cli");
    thread_ticks = 0; // Reset quantum on yield
    schedule();
    __asm__ volatile("sti");
}

void thread_exit(void) {
    __asm__ volatile("cli");
    current_thread->state = THREAD_TERMINATED;
    thread_ticks = 0;
    schedule();
    // Should never reach here
    for (;;);
}

void thread_block(thread_state_t block_state) {
    __asm__ volatile("cli");
    current_thread->state = block_state;
    thread_ticks = 0;
    schedule();
    __asm__ volatile("sti");
}

void thread_wake(thread_t* thread) {
    __asm__ volatile("cli");
    if (thread->state == THREAD_BLOCKED || thread->state == THREAD_BLOCKED_KEYBOARD) {
        thread->state = THREAD_READY;
    }
    __asm__ volatile("sti");
}

thread_t* scheduler_get_current_thread(void) {
    return current_thread;
}

void scheduler_wake_keyboard_waiters(void) {
    thread_t* curr = thread_list;
    while (curr != NULL) {
        if (curr->state == THREAD_BLOCKED_KEYBOARD) {
            curr->state = THREAD_READY;
        }
        curr = curr->next;
    }
}

static void pit_interrupt_handler(registers_t* regs) {
    (void)regs;
    thread_ticks++;
    if (thread_ticks >= QUANTUM) {
        thread_ticks = 0;
        schedule();
    }
}

void pit_init(int hz) {
    int divisor = 1193180 / hz;
    outb(0x43, 0x36);                   // Command byte: channel 0, LSB then MSB, rate generator
    outb(0x40, divisor & 0xFF);         // LSB
    outb(0x40, (divisor >> 8) & 0xFF);   // MSB

    register_interrupt_handler(32, pit_interrupt_handler);
    irq_clear_mask(0); // Unmask IRQ0

    vga_printf("PIT: Programmed to %d Hz (IRQ 0 unmasked).\n", hz);
}
