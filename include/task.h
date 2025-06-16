#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define MAX_TASKS 8
#define TASK_STACK_SIZE 8192  // 8KB stack per task

// Task states
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_TERMINATED
} task_state_t;

// Task structure
struct task {
    uint32_t id;
    task_state_t state;
    void (*entry)(void);
    uint64_t rsp;                           // Stack pointer
    uint8_t stack[TASK_STACK_SIZE];        // Task stack
};

// External declarations
extern struct task *current_task;
extern struct task tasks[MAX_TASKS];
extern int num_tasks;

// Function prototypes
void task_init(void);
void task_create(void (*entry)(void));
void schedule(void);
void task_yield(void);
void task_sleep(uint32_t ticks);

#endif