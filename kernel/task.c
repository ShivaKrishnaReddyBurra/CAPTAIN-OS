#include "task.h"
#include "vga.h"

struct task *current_task = 0;
struct task tasks[MAX_TASKS];
int num_tasks = 0;

// Assembly function to switch tasks (defined in isr.asm)
extern void switch_task(uint64_t *old_rsp, uint64_t new_rsp);

void task_init(void) {
    num_tasks = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].state = TASK_BLOCKED;  // Mark as unused
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

    // Set up the stack: rsp points to the top of the stack, ensure 16-byte alignment
    task->rsp = (uint64_t)&task->stack[TASK_STACK_SIZE / 8];
    // Align to 16-byte boundary (required for x86_64)
    task->rsp &= ~(uint64_t)0xF;

    // Initialize the stack frame for the first context switch (for iretq)
    uint64_t *stack = (uint64_t *)task->rsp;
    *--stack = 0x10;                // SS (stack segment)
    *--stack = task->rsp;           // RSP (initial stack pointer for the task)
    *--stack = 0x202;               // RFLAGS (interrupts enabled, but we'll manage this carefully)
    *--stack = 0x08;                // CS (code segment)
    *--stack = (uint64_t)entry;     // RIP (entry point)
    *--stack = 0;                   // RAX
    *--stack = 0;                   // RBX
    *--stack = 0;                   // RCX
    *--stack = 0;                   // RDX
    *--stack = 0;                   // RSI
    *--stack = 0;                   // RDI
    *--stack = 0;                   // RBP
    *--stack = 0;                   // R8
    *--stack = 0;                   // R9
    *--stack = 0;                   // R10
    *--stack = 0;                   // R11
    *--stack = 0;                   // R12
    *--stack = 0;                   // R13
    *--stack = 0;                   // R14
    *--stack = 0;                   // R15
    task->rsp = (uint64_t)stack;

    num_tasks++;
}

void schedule(void) {
    if (num_tasks == 0) return;

    // If we don't have a current task, start with task 0
    if (!current_task) {
        current_task = &tasks[0];
        if (current_task->state == TASK_READY) {
            current_task->state = TASK_RUNNING;
            // First task: jump directly to its entry point
            register uint64_t rsp asm("rsp") = current_task->rsp;
            __asm__ volatile(
                "movq %0, %%rsp\n"
                "popq %%r15\n"
                "popq %%r14\n"
                "popq %%r13\n"
                "popq %%r12\n"
                "popq %%r11\n"
                "popq %%r10\n"
                "popq %%r9\n"
                "popq %%r8\n"
                "popq %%rbp\n"
                "popq %%rdi\n"
                "popq %%rsi\n"
                "popq %%rdx\n"
                "popq %%rcx\n"
                "popq %%rbx\n"
                "popq %%rax\n"
                "iretq"
                : "+r"(rsp)
                :
                : "memory", "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
                  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
            );
        }
        return;
    }

    // Find the next ready task (round-robin)
    int next_task_id = (current_task->id + 1) % num_tasks;
    struct task *next_task = &tasks[next_task_id];

    // Skip tasks that aren't ready
    int tries = 0;
    while (next_task->state != TASK_READY && tries < num_tasks) {
        next_task_id = (next_task_id + 1) % num_tasks;
        next_task = &tasks[next_task_id];
        tries++;
    }

    // If no other ready tasks, stay with current task
    if (next_task->state != TASK_READY || next_task == current_task) {
        return;
    }

    // Save current task state
    if (current_task->state == TASK_RUNNING) {
        current_task->state = TASK_READY;
    }

    // Switch to the next task
    struct task *old_task = current_task;
    current_task = next_task;
    current_task->state = TASK_RUNNING;

    // Perform the actual context switch
    switch_task(&old_task->rsp, current_task->rsp);
}