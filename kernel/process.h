#ifndef PROCESS_H
#define PROCESS_H

#include "paging.h"
#include "thread.h"

typedef enum {
    PROC_READY,
    PROC_RUNNING,
    PROC_TERMINATED
} proc_state_t;

struct process_struct {
    int pid;
    int parent_pid;
    proc_state_t state;
    page_directory_t* page_directory; // Physical address of the page directory
    thread_t* main_thread;
};

typedef struct process_struct process_t;

void process_init(void);
process_t* process_create(void* code_src, unsigned int code_size);

#endif
