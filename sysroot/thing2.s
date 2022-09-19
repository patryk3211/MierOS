[bits 64]
[org 0x1000000]
_start:
    ; Open the file
    mov rax, 2
    mov rbx, filename
    int 0x8F

    ; Store fd
    mov r15, rax

    ; Mmap
    mov rax, 8
    mov rbx, 0
    mov rcx, 4096
    mov rdx, 3
    mov rsi, 1
    mov rdi, r15
    mov r8, 0
    int 0x8F

    ; Store address
    mov r14, rax

    ; Close the file
    mov rax, 3
    mov rbx, r15
    int 0x8F

    ; Move first 8 bytes into r13
    mov r13, [r14]
    mov [buffer], r13

    ; Print the first 8 bytes
    mov rax, 5
    mov rbx, 1
    mov rcx, buffer
    mov rdx, 8
    int 0x8F

    ; Execute 'thing3'
    mov rax, 10
    mov rbx, execfile
    mov rcx, 0
    mov rdx, 0
    int 0x8F

    ; Exit
    mov rax, 1
    mov rbx, 0
    int 0x8F

section .data
filename: db "/thing2.s", 0
execfile: db "/thing3", 0

section .bss
buffer: resb 8
