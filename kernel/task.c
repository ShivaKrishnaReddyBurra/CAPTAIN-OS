#include "task.h"
#include "vga.h"
#include "utils.h"
#include <stddef.h>

struct task *current_task = 0;
struct task tasks[MAX_TASKS];
int num_tasks = 0;
static int scheduler_initialized = 0;
static int in_critical_section = 0;

extern void simple_switch_task(uint64_t *old_rsp, uint64_t new_rsp);

void task_wrapper(void) {
    void (*entry)(void) = current_task->entry;
    entry();
    if (current_task) {
        current_task->state = TASK_BLOCKED;
    }
    while (1) {
        schedule();
        __asm__ volatile("hlt");
    }
}

void task_init(void) {
    num_tasks = 0;
    current_task = 0;
    scheduler_initialized = 0;
    in_critical_section = 0;
    
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

    uint8_t *stack_base = (uint8_t *)task->stack;
    uint64_t *stack_top = (uint64_t *)(stack_base + TASK_STACK_SIZE);
    stack_top = (uint64_t *)((uint64_t)stack_top & ~0xF);
    
    *(--stack_top) = (uint64_t)task_wrapper;
    *(--stack_top) = 0;
    *(--stack_top) = 0;
    *(--stack_top) = 0;
    *(--stack_top) = 0;
    *(--stack_top) = 0;
    *(--stack_top) = 0;
    *(--stack_top) = 0x202;
    task->rsp = (uint64_t)stack_top;
    
    num_tasks++;
}

void enter_critical_section(void) {
    __asm__ volatile("cli");
    in_critical_section = 1;
}

void exit_critical_section(void) {
    in_critical_section = 0;
    __asm__ volatile("sti");
}

int is_in_critical_section(void) {
    return in_critical_section;
}

void schedule(void) {
    if (num_tasks == 0) return;
    if (in_critical_section) return;
    __asm__ volatile("cli");

    if (!scheduler_initialized) {
        scheduler_initialized = 1;
        for (int i = 0; i < num_tasks; i++) {
            if (tasks[i].state == TASK_READY) {
                current_task = &tasks[i];
                current_task->state = TASK_RUNNING;
                print_string("First task: ");
                char buf[16];
                itoa(current_task->id, buf, 10);
                print_string(buf);
                print_string("\n");
                __asm__ volatile("sti");
                return;
            }
        }
        print_string("No ready tasks!\n");
        __asm__ volatile("sti");
        return;
    }

    int start_id = current_task->id;
    int next_id = (start_id + 1) % num_tasks;
    struct task *next_task = NULL;
    for (int i = 0; i < num_tasks; i++) {
        if (tasks[next_id].state == TASK_READY) {
            next_task = &tasks[next_id];
            break;
        }
        next_id = (next_id + 1) % num_tasks;
    }

    if (!next_task) {
        if (current_task->state == TASK_RUNNING) {
            __asm__ volatile("sti");
            return;
        }
        print_string("No ready tasks to switch to!\n");
        current_task = 0;
        __asm__ volatile("sti");
        return;
    }

    if (next_task == current_task) {
        __asm__ volatile("sti");
        return;
    }

    struct task *old_task = current_task;
    if (old_task->state == TASK_RUNNING) {
        old_task->state = TASK_READY;
    }
    current_task = next_task;
    current_task->state = TASK_RUNNING;

    print_string("Switching from task ");
    char buf[16];
    itoa(old_task->id, buf, 10);
    print_string(buf);
    print_string(" to task ");
    itoa(current_task->id, buf, 10);
    print_string(buf);
    print_string(" (Shell:0, BG:1, Idle:2)\n");

    simple_switch_task(&old_task->rsp, current_task->rsp);
    __asm__ volatile("sti");
}

void task_yield(void) {
    if (current_task && current_task->state == TASK_RUNNING) {
        current_task->state = TASK_READY;
    }
    schedule();
}

void task_sleep(uint32_t ticks) {
    for (uint32_t i = 0; i < ticks; i++) {
        task_yield();
        for (volatile int j = 0; j < 1000; j++);
    }
}

void start_multitasking(void) {
    if (num_tasks == 0) {
        print_string("No tasks to start!\n");
        return;
    }
    
    struct task *first_task = NULL;
    for (int i = 0; i < num_tasks; i++) {
        if (tasks[i].state == TASK_READY) {
            first_task = &tasks[i];
            break;
        }
    }
    
    if (!first_task) {
        print_string("No ready tasks!\n");
        return;
    }
    
    current_task = first_task;
    current_task->state = TASK_RUNNING;
    scheduler_initialized = 1;
    
    __asm__ volatile(
        "mov %0, %%rsp\n"
        "popfq\n"
        "pop %%r15\n"
        "pop %%r14\n"
        "pop %%r13\n"
        "pop %%r12\n"
        "pop %%rbx\n"
        "pop %%rbp\n"
        "ret\n"
        :
        : "r" (current_task->rsp)
        : "memory"
    );
}