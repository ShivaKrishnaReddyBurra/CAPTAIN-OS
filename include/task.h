#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define MAX_TASKS 10
#define TASK_STACK_SIZE 4096

// Task states
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED
} task_state_t;

// Task structure
struct task {
    uint32_t id;
    task_state_t state;
    uint64_t rsp;  // Stack pointer
    void (*entry)(void);  // Entry point function
    uint8_t stack[TASK_STACK_SIZE];  // Task stack
};

// Global current task pointer
extern struct task *current_task;

// Task management functions
void task_init(void);
void task_create(void (*entry)(void));
void schedule(void);
void task_yield(void);
void task_sleep(uint32_t ticks);
void start_multitasking(void);

// Critical section management
void enter_critical_section(void);
void exit_critical_section(void);
int is_in_critical_section(void);

#endif