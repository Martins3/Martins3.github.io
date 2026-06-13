section .text
    global _start
    _start:                 ; ELF entry point
     ; 1 is the number for syscall write ().

    mov rax, 1              
    ; 1 is the STDOUT file descriptor.

    mov rdi, 1              

    ; buffer to be printed.

    mov rsi, message        

    ; length of buffer

    mov rdx, [messageLen]       

    ; call the syscall instruction
    syscall
    
    ; sys_exit
    mov rax, 60

    ; return value is 0
    mov rdi, 0

    ; call the assembly instruction
    syscall
    
section .data
    messageLen: dq message.end-message
    message: db 'Hello World', 10
.end:
