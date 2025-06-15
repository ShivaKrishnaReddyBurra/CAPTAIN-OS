#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define MAX_TASKS 10
#define TASK_STACK_SIZE 4096  // 4KB stack per task

// Task states
enum task_state {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED
};

// Task Control Block (TCB)
struct task {
    uint64_t rsp;              // Stack pointer (saved during context switch)
    uint64_t stack[TASK_STACK_SIZE / 8];  // Task stack (4KB, aligned to 8 bytes)
    enum task_state state;     // Task state
    void (*entry)(void);       // Task entry point
    uint32_t id;               // Task ID
};

extern struct task *current_task;
extern struct task tasks[MAX_TASKS];
extern int num_tasks;

void task_init(void);
void schedule(void);
void task_create(void (*entry)(void));

#endif