#ifndef THREAD_H
#define THREAD_H

#include <stddef.h>

typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_BLOCKED_KEYBOARD,
    THREAD_TERMINATED
} thread_state_t;

struct process_struct; // Forward declaration

struct thread_struct {
    void* esp;                      // Saved stack pointer (MUST be the first member)
    int tid;
    thread_state_t state;
    void* stack_limit;              // Pointer to start of allocated stack
    unsigned int stack_size;
    struct thread_struct* next;
    struct process_struct* process; // Parent process
};

typedef struct thread_struct thread_t;

void scheduler_init(void);
thread_t* thread_create(void (*entry_fn)(void), unsigned int stack_size);
void thread_yield(void);
void thread_exit(void);
void thread_block(thread_state_t block_state);
void thread_wake(thread_t* thread);

thread_t* scheduler_get_current_thread(void);
void scheduler_wake_keyboard_waiters(void);
void pit_init(int hz);

extern int scheduling_trace;

#endif
