[bits 64]
[org 0x1000000]
_start:
    ; Write 'Hello, World!'
    mov eax, 5
    mov ebx, 1
    mov ecx, msg
    mov edx, [msg_len]
    int 0x8F

    ; mmap
    mov rax, 8
    mov rbx, 0
    mov rcx, 4096
    mov rdx, 3
    mov rsi, 2
    mov rdi, 0
    mov r8, 0
    int 0x8F

    ; Fork
    mov eax, 6
    int 0x8F

    cmp eax, 0
    jne .child
.parent:
    mov ecx, msg_parent
    mov edx, [msg_parent_len]
    jmp .child.post
.child:
    mov ecx, msg_child
    mov edx, [msg_child_len]
.child.post:
    mov eax, 5
    mov ebx, 1
    int 0x8F

    ; Exit
    mov eax, 1
    mov ebx, 0
    int 0x8F

msg: db "Hello, World!", 10
msg_len: dd $ - msg
msg_parent: db "Hello, Parent Process!", 10
msg_parent_len: dd $ - msg_parent
msg_child: db "Hello, Child Process!", 10
msg_child_len: dd $ - msg_child
