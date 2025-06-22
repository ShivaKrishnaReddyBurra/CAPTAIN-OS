section .multiboot
align 8
multiboot2_header_start:
    dd 0xE85250D6            ; Multiboot2 magic number
    dd 0                     ; Architecture: 0 for i386 (32-bit)
    dd multiboot2_header_end - multiboot2_header_start ; Header length
    dd -(0xE85250D6 + 0 + (multiboot2_header_end - multiboot2_header_start)) ; Checksum

    ; Framebuffer tag - request graphics mode
    ; align 8
    ; dw 5                     ; Type: Framebuffer tag
    ; dw 0                     ; Flags: 0 (required)
    ; dd 20                    ; Size of this tag
    ; dd 800                  ; Width
    ; dd 600                   ; Height  
    ; dd 32                    ; Depth (bits per pixel)

    ; Information request tag
    align 8
    dw 1                     ; Type: Information request
    dw 0                     ; Flags: 0 (optional)
    dd 8                     ; Size of this tag

    ; End tag (required)
    align 8
    dw 0                     ; Type: 0 (end tag)
    dw 0                     ; Flags: 0
    dd 8                     ; Size of end tag
multiboot2_header_end:

section .text
bits 32                          ; Start in 32-bit mode (GRUB boots in 32-bit)
global _start
extern kernel_main               ; Entry point to C kernel
_start:
    cli                          ; Disable interrupts
    mov esp, stack_top           ; Set up stack pointer (32-bit)

    call setup_64bit             ; Set up 64-bit mode
    jmp 0x08:higher_half         ; Far jump to 64-bit code segment (selector 0x08)

setup_64bit:
    ; Disable paging
    mov eax, cr0
    and eax, 0x7FFFFFFF          ; Clear PG bit (bit 31)
    mov cr0, eax

    ; Set up a minimal GDT for 64-bit mode
    lgdt [gdt_descriptor]

    ; Enable PAE (Physical Address Extension)
    mov eax, cr4
    or eax, 1 << 5               ; Set PAE bit
    mov cr4, eax

    ; Set the long mode bit in the EFER MSR
    mov ecx, 0xC0000080          ; EFER MSR
    rdmsr
    or eax, 1 << 8               ; Set LME bit (Long Mode Enable)
    wrmsr

    ; Enable paging (minimal for now, identity mapping)
    mov eax, pml4                ; Point to a basic PML4 table
    mov cr3, eax

    mov eax, cr0
    or eax, 1 << 31              ; Set PG bit to enable paging
    mov cr0, eax

    ret

section .text
bits 64                          ; Switch to 64-bit mode for this section
higher_half:
    ; Now in 64-bit mode
    mov rsp, stack_top           ; Use 64-bit stack pointer
    lgdt [gdt_descriptor]        ; Reload GDT in 64-bit mode

    ; Reload segment registers
    mov ax, 0x10                 ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    call kernel_main             ; Call kernel
    hlt                          ; Halt CPU if kernel returns

section .data
align 16
gdt:
    ; Null descriptor
    dq 0
    ; 64-bit code segment (selector 0x08)
    dq 0x00AF9A000000FFFF        ; Base=0, Limit=0xFFFFF, Present, Code, 64-bit
    ; 64-bit data segment (selector 0x10)
    dq 0x00CF92000000FFFF        ; Base=0, Limit=0xFFFFF, Present, Data

gdt_descriptor:
    dw gdt_end - gdt - 1         ; GDT size
    dq gdt                       ; GDT address (64-bit address for 64-bit mode)

; Minimal paging structures (identity map the first 2MB)
align 4096
pml4:
    dq pdpt + 0x07               ; Present, Read/Write
    times 511 dq 0               ; Fill rest with zeros
pdpt:
    dq pdt + 0x07                ; Present, Read/Write
    times 511 dq 0
pdt:
    dq 0x0000000000000087        ; Map first 2MB
    dq 0x0000000020000087        ; Map next 2MB
    ; Add more entries for 1GB (512 entries * 2MB = 1GB)
    times 510 dq 0

gdt_end:

section .bss
align 16
stack_bottom:
    resb 8192                    ; Reserve 8KB for stack
stack_top: