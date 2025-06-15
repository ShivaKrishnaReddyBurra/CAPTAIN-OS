section .text
bits 64

global load_idt
load_idt:
    mov rax, rdi
    lidt [rax]
    ret

global switch_task
switch_task:
    ; Parameters:
    ; rdi: pointer to old_rsp (where to save current rsp)
    ; rsi: new_rsp (value to load into rsp)

    ; Save current registers
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    ; Save current rsp to old_rsp
    mov [rdi], rsp

    ; Load new rsp
    mov rsp, rsi

    ; Restore registers for the new task
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    ret

global timer_isr
extern timer_handler
timer_isr:
    ; Save all registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    ; Align stack to 16-byte boundary  
    mov rbp, rsp
    and rsp, -16
    
    ; Debug: Confirm timer interrupt
    mov rdi, timer_msg
    ; call print_string
    
    ; Call handler
    call timer_handler
    
    ; Restore stack
    mov rsp, rbp
    
    ; Restore all registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    iretq

global keyboard_isr
extern keyboard_handler
keyboard_isr:
    ; Save all registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    ; Align stack to 16-byte boundary
    mov rbp, rsp
    and rsp, -16
    
    ; Debug: Confirm keyboard interrupt
    mov rdi, keyboard_msg
    ; call print_string
    
    ; Call handler
    call keyboard_handler
    
    ; Restore stack
    mov rsp, rbp
    
    ; Restore all registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    iretq

global inb
inb:
    mov dx, di
    in al, dx
    ret

global outb
outb:
    mov dx, di
    mov al, sil
    out dx, al
    ret

global exception_isr
extern exception_handler
exception_isr:
    ; Save registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    
    ; Align stack
    mov rbp, rsp
    and rsp, -16
    
    call exception_handler
    
    ; Restore stack and registers
    mov rsp, rbp
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    iretq

section .data
timer_msg db "Timer interrupt\n", 0
keyboard_msg db "Keyboard ISR called\n", 0