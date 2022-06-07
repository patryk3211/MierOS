[bits 64]
[org 0x1000000]
_start:
    mov eax, 5
    mov ebx, 1
    mov ecx, msg
    mov edx, [msg_len]
    int 0x8F

    mov eax, 1
    mov ebx, 0
    int 0x8F

    jmp $

msg: db "Hello, World!", 10
msg_len: dd $ - msg
