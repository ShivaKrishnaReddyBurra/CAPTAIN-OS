section .text
bits 64

global load_idt
load_idt:
    mov rax, rdi
    lidt [rax]
    ret

; Fixed task switching with proper register preservation and stack handling
global simple_switch_task
simple_switch_task:
    ; Parameters:
    ; rdi: pointer to old_rsp (where to save current RSP)
    ; rsi: new_rsp (RSP to switch to)
    
    ; Save caller's flags
    pushfq
    
    ; Save all callee-saved registers in the correct order
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    ; Save current stack pointer to old task
    mov [rdi], rsp
    
    ; Switch to new task's stack
    mov rsp, rsi
    
    ; Restore callee-saved registers for new task (in reverse order)
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    
    ; Restore flags
    popfq
    
    ; Return to new task (this will jump to task_wrapper for new tasks)
    ret

global timer_isr
extern timer_handler
timer_isr:
    ; Save all registers
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    
    ; Call the C handler
    call timer_handler
    
    ; Restore all registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    
    ; Return from interrupt
    iretq

global keyboard_isr
extern keyboard_handler
keyboard_isr:
    ; Save all registers
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    
    ; Call the C handler
    call keyboard_handler
    
    ; Restore all registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    
    ; Return from interrupt
    iretq

global inb
inb:
    mov dx, di      ; Port number to DX
    in al, dx       ; Read byte from port
    ret

global outb
outb:
    mov dx, di      ; Port number to DX
    mov al, sil     ; Value to AL
    out dx, al      ; Write byte to port
    ret

global exception_isr
extern exception_handler
exception_isr:
    ; Save all registers
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    ; Call exception handler
    call exception_handler
    
    ; Restore all registers (though we probably won't return)
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    
    ; Return from interrupt (if we ever get here)
    iretq