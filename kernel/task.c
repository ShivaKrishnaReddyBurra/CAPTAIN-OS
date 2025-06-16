#include "task.h"
#include "vga.h"
#include <stddef.h>

struct task *current_task = 0;
struct task tasks[MAX_TASKS];
int num_tasks = 0;
static int scheduler_initialized = 0;
static uint32_t task_switch_counter = 0;

// Simple task switching - save/restore RSP and basic registers
extern void simple_switch_task(uint64_t *old_rsp, uint64_t new_rsp);

// Task wrapper function - ensures tasks never exit
void task_wrapper(void (*entry)(void)) {
    // Enable interrupts for this task
    __asm__ volatile("sti");
    
    entry();
    
    // If task ever returns, put it in blocked state and yield
    if (current_task) {
        current_task->state = TASK_BLOCKED;
    }
    while (1) {
        schedule(); // Try to switch to another task
        __asm__ volatile("hlt");
    }
}

void task_init(void) {
    num_tasks = 0;
    current_task = 0;
    scheduler_initialized = 0;
    task_switch_counter = 0;
    
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].state = TASK_BLOCKED;
        tasks[i].id = i;
        tasks[i].entry = 0;
        tasks[i].rsp = 0;
    }
}

void task_create(void (*entry)(void)) {
    if (num_tasks >= MAX_TASKS) {
        print_string("Error: Max tasks reached!\n");
        return;
    }

    struct task *task = &tasks[num_tasks];
    task->id = num_tasks;
    task->entry = entry;
    task->state = TASK_READY;

    // Set up stack properly - much more careful stack setup
    uint8_t *stack_base = (uint8_t *)task->stack;
    uint64_t *stack_top = (uint64_t *)(stack_base + TASK_STACK_SIZE);
    
    // Ensure 16-byte alignment (required by x86-64 ABI)
    stack_top = (uint64_t *)((uint64_t)stack_top & ~0xF);
    
    // Set up the stack frame more carefully
    // We need to simulate a proper function call stack
    
    // Push the entry point as a parameter for task_wrapper
    *(--stack_top) = (uint64_t)entry;
    
    // Push return address (task_wrapper)
    *(--stack_top) = (uint64_t)task_wrapper;
    
    // Push fake return address for task_wrapper (should never be used)
    *(--stack_top) = 0;
    
    // Set up saved registers that simple_switch_task expects
    // These will be popped in reverse order
    *(--stack_top) = 0; // r15
    *(--stack_top) = 0; // r14
    *(--stack_top) = 0; // r13
    *(--stack_top) = 0; // r12
    *(--stack_top) = 0; // rbx
    *(--stack_top) = 0; // rbp
    
    // Set the task's stack pointer
    task->rsp = (uint64_t)stack_top;
    
    num_tasks++;
}

void schedule(void) {
    if (num_tasks == 0) return;
    
    // Disable interrupts during scheduling
    __asm__ volatile("cli");

    // Handle first task startup
    if (!scheduler_initialized) {
        scheduler_initialized = 1;
        if (num_tasks > 0) {
            current_task = &tasks[0];
            current_task->state = TASK_RUNNING;
            __asm__ volatile("sti");
            return;
        }
    }

    // Control task switching frequency
    task_switch_counter++;
    if (task_switch_counter < 20) { // Reduced frequency for stability
        __asm__ volatile("sti");
        return;
    }
    task_switch_counter = 0;

    // If no current task, find any ready task
    if (!current_task) {
        for (int i = 0; i < num_tasks; i++) {
            if (tasks[i].state == TASK_READY) {
                current_task = &tasks[i];
                current_task->state = TASK_RUNNING;
                __asm__ volatile("sti");
                return;
            }
        }
        __asm__ volatile("sti");
        return;
    }

    // Find next ready task using round-robin scheduling
    int start_id = current_task->id;
    int next_id = (start_id + 1) % num_tasks;
    struct task *next_task = NULL;
    
    // Look for a ready task, starting from the next task
    for (int i = 0; i < num_tasks; i++) {
        if (tasks[next_id].state == TASK_READY) {
            next_task = &tasks[next_id];
            break;
        }
        next_id = (next_id + 1) % num_tasks;
    }

    // If no ready task found, keep current task running if possible
    if (!next_task) {
        if (current_task->state == TASK_RUNNING) {
            __asm__ volatile("sti");
            return; // Keep current task running
        } else {
            // Current task is blocked, look for any ready task
            for (int i = 0; i < num_tasks; i++) {
                if (tasks[i].state == TASK_READY) {
                    next_task = &tasks[i];
                    break;
                }
            }
            if (!next_task) {
                current_task = 0; // No tasks ready
                __asm__ volatile("sti");
                return;
            }
        }
    }

    // If it's the same task, no switch needed
    if (next_task == current_task) {
        __asm__ volatile("sti");
        return;
    }

    // Perform task switch
    struct task *old_task = current_task;
    
    // Update states
    if (old_task->state == TASK_RUNNING) {
        old_task->state = TASK_READY;
    }
    
    current_task = next_task;
    current_task->state = TASK_RUNNING;

    // Perform context switch - interrupts will be re-enabled after switch
    simple_switch_task(&old_task->rsp, current_task->rsp);
    
    __asm__ volatile("sti");
}

void task_yield(void) {
    // Allow a task to voluntarily give up the CPU
    if (current_task && current_task->state == TASK_RUNNING) {
        current_task->state = TASK_READY;
    }
    schedule();
}

void task_sleep(uint32_t ticks) {
    // Simple sleep implementation - just yield CPU for a number of scheduler calls
    for (uint32_t i = 0; i < ticks; i++) {
        task_yield();
        // Add a small delay
        for (volatile int j = 0; j < 1000; j++);
    }
}