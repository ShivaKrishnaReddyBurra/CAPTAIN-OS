section .multiboot
align 8
multiboot2_header_start:
    dd 0xe85250d6                ; Multiboot 2 magic number
    dd 0                         ; Architecture: 0 for i386 (protected mode)
    dd multiboot2_header_end - multiboot2_header_start  ; Header length
    dd 0x100000000 - (0xe85250d6 + 0 + (multiboot2_header_end - multiboot2_header_start))  ; Checksum

    ; End tag (required)
    dw 0                         ; Type: 0 (end tag)
    dw 0                         ; Flags: 0
    dd 8                         ; Size: 8 bytes
multiboot2_header_end:

section .text
bits 32                          ; Start in 32-bit mode (multiboot requirement)
global _start
extern kernel_main

_start:
    cli                          ; Disable interrupts
    mov esp, stack_top           ; Set up stack pointer (32-bit)
    
    ; First, let's try to display something in 32-bit mode to debug
    mov edi, 0xB8000            ; VGA buffer
    mov eax, 0x0F300F32         ; '2' and '3' with white on black attribute
    mov [edi], eax              ; Write to screen
    
    ; Check if CPUID is supported
    pushfd                       ; Save EFLAGS
    pop eax                      ; Get EFLAGS into EAX
    mov ecx, eax                 ; Save original EFLAGS
    xor eax, 1 << 21             ; Flip the ID bit
    push eax                     ; Push modified EFLAGS
    popfd                        ; Load modified EFLAGS
    pushfd                       ; Save EFLAGS again
    pop eax                      ; Get EFLAGS
    push ecx                     ; Restore original EFLAGS
    popfd
    cmp eax, ecx                 ; Compare with original
    je no_64bit                  ; If same, no CPUID support

    ; Check if long mode is available
    mov eax, 0x80000000          ; Get highest extended function
    cpuid
    cmp eax, 0x80000001          ; Check if extended function 0x80000001 is available
    jb no_64bit                  ; If not, no long mode

    mov eax, 0x80000001          ; Extended function 0x80000001
    cpuid
    test edx, 1 << 29            ; Check LM bit (bit 29)
    jz no_64bit                  ; If not set, no long mode

    ; Display '6' to show we got to 64-bit setup
    mov dword [0xB8004], 0x0F360F34  ; '4' and '6'

    ; Set up page tables
    ; Clear the page table area
    mov edi, 0x1000             ; Start of page tables
    mov ecx, 0x3000             ; Clear 12KB (3 pages)
    xor eax, eax
    rep stosb
    
    ; Set up PML4 (Page Map Level 4)
    mov edi, 0x1000             ; PML4 at 0x1000
    mov dword [edi], 0x2003     ; Point to PDPT at 0x2000, Present + Writable
    
    ; Set up PDPT (Page Directory Pointer Table)
    mov edi, 0x2000             ; PDPT at 0x2000
    mov dword [edi], 0x3003     ; Point to PD at 0x3000, Present + Writable
    
    ; Set up PD (Page Directory) with 2MB pages
    mov edi, 0x3000             ; PD at 0x3000
    mov eax, 0x00000083         ; 0x0000000 | Present + Writable + Huge Page
    mov ecx, 512                ; 512 entries * 2MB = 1GB mapped
.set_pd_entry:
    mov [edi], eax
    add eax, 0x200000           ; Next 2MB page
    add edi, 8                  ; Next entry
    loop .set_pd_entry
    
    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5              ; Set PAE bit
    mov cr4, eax
    
    ; Set CR3 to point to PML4
    mov eax, 0x1000
    mov cr3, eax
    
    ; Enable long mode in EFER
    mov ecx, 0xC0000080         ; EFER MSR
    rdmsr
    or eax, 1 << 8              ; Set Long Mode Enable bit
    wrmsr
    
    ; Enable paging (this activates long mode)
    mov eax, cr0
    or eax, 1 << 31             ; Set Paging Enable bit
    mov cr0, eax
    
    ; Load 64-bit GDT
    lgdt [gdt64.pointer]
    
    ; Display '8' to show we're about to jump to 64-bit
    mov dword [0xB8008], 0x0F380F38  ; '8' and '8'
    
    ; Jump to 64-bit code segment
    jmp gdt64.code:long_mode_start
    
no_64bit:
    ; Display error and halt
    mov dword [0xB8000], 0x4E4F4E4F  ; 'NO' in red
    mov dword [0xB8004], 0x36344E36  ; '64' in red
    hlt

bits 64
long_mode_start:
    ; Clear segment registers
    mov ax, gdt64.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Set up 64-bit stack
    mov rsp, stack_top
    
    ; Display 'OK' to show we're in 64-bit mode
    mov rax, 0x0F4B0F4F         ; 'OK' in white
    mov [0xB800C], rax
    
    ; Call kernel
    call kernel_main
    
.hang:
    hlt
    jmp .hang

section .rodata
align 8
gdt64:
    dq 0                        ; Null descriptor
.code: equ $ - gdt64
    dq 0x00209A0000000000       ; 64-bit code descriptor (executable/readable)
.data: equ $ - gdt64  
    dq 0x0000920000000000       ; 64-bit data descriptor (writable)
.pointer:
    dw $ - gdt64 - 1            ; Limit
    dq gdt64                    ; Base

section .bss
align 16
stack_bottom:
    resb 65536                  ; 64KB stack
stack_top: