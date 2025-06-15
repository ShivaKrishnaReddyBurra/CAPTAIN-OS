#include <stdint.h>
#include "idt.h"
#include "vga.h"
#include "utils.h"

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

#define ICW1_ICW4       0x01    /* ICW4 (not) needed */
#define ICW1_SINGLE     0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04    /* Call address interval 4 (8) */
#define ICW1_LEVEL      0x08    /* Level triggered (edge) mode */
#define ICW1_INIT       0x10    /* Initialization - required! */

#define ICW4_8086       0x01    /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02    /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08    /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C    /* Buffered mode/master */
#define ICW4_SFNM       0x10    /* Special fully nested (not) */

void pic_remap(void) {
    // Save current masks
    uint8_t pic1_mask = inb(PIC1_DATA);
    uint8_t pic2_mask = inb(PIC2_DATA);

    // Start initialization sequence
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    // Set vector offsets
    outb(PIC1_DATA, 0x20);  // PIC1 starts at INT 0x20
    outb(PIC2_DATA, 0x28);  // PIC2 starts at INT 0x28

    // Configure cascade
    outb(PIC1_DATA, 0x04);  // PIC1 has slave at IRQ2
    outb(PIC2_DATA, 0x02);  // PIC2 cascade identity

    // Set mode
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // Mask all interrupts except timer (IRQ0) and keyboard (IRQ1)
    outb(PIC1_DATA, 0xFC);  // Enable IRQ0 (timer) and IRQ1 (keyboard) only
    outb(PIC2_DATA, 0xFF);  // Disable all IRQ8-15
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, 0x20);  // Send EOI to slave PIC
    }
    outb(PIC1_COMMAND, 0x20);      // Send EOI to master PIC
}