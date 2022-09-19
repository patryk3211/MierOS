section .text
global _start
_start:
    mov rax, 5
    mov rbx, 1
    mov rcx, helloMsg
    mov rdx, [helloMsgLen]
    int 0x8F

    mov rax, 1
    mov rbx, 0
    int 0x8F

section .rodata
helloMsg: db "Hello from an executed Elf File!"
helloMsgLen: dq $ - helloMsg
