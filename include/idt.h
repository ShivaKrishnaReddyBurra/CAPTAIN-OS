#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define IDT_ENTRIES 256

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// I/O port functions (from isr.asm)
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);

// PIC functions
void pic_remap(void);
void pic_send_eoi(uint8_t irq);

void idt_init(void);
void keyboard_handler(void);
void timer_handler(void);
void enable_keyboard(void);

#endif