#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_WRITE  1
#define SYS_YIELD  2
#define SYS_EXIT   3
#define SYS_SBRK   4

void syscall_init(void);

#endif
