ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)  /* Explicitly specify 64-bit ELF output */
SECTIONS {
    . = 0x100000; /* Kernel loaded at 1MB */

    /* Ensure the multiboot section is the very first thing in the binary */
    .multiboot 0x100000 : {
        KEEP(*(.multiboot))  /* Keep the multiboot section even if unused */
    }

    /* Place the text section immediately after */
    .text : {
        *(.text*)
    }

    /* Data section */
    .data : {
        *(.data)
    }

    /* BSS section */
    .bss : {
        *(.bss)
    }

    /* Ensure the binary size is at least 32KB to include the multiboot header */
    . = . + 0x8000;
}