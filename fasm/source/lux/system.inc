; flat assembler interface for lux x64
; Copyright (c) 1999-2022, Tomasz Grysztar.
;
; Interface by Omar Elghoul, 2025

segment readable executable

; lux-specific constants
SYSCALL_EXIT            = 0
SYSCALL_OPEN            = 14
SYSCALL_CLOSE           = 15
SYSCALL_READ            = 16
SYSCALL_WRITE           = 17
SYSCALL_FSTAT           = 19
SYSCALL_LSEEK           = 20
SYSCALL_SBRK            = 54

; open() flags
O_RDONLY                = 0x0004
O_WRONLY                = 0x0008
O_RDWR                  = 0x000C
O_APPEND                = 0x0010
O_CREAT                 = 0x0020
O_EXCL                  = 0x0080
O_TRUNC                 = 0x0800

; stat permissions
S_IRUSR                 = 0x0008
S_IWUSR                 = 0x0010
S_IXUSR                 = 0x0020
S_IRGRP                 = 0x0040
S_IWGRP                 = 0x0080
S_IXGRP                 = 0x0100
S_IROTH                 = 0x0200
S_IWOTH                 = 0x0400
S_IXOTH                 = 0x0800

; index of file size within struct stat
ST_SIZE                 = 48

STDIN_FILENO            = 0
STDOUT_FILENO           = 1
STDERR_FILENO           = 2

SEEK_SET                = 1
SEEK_CUR                = 2
SEEK_END                = 3

; exit_program(): exits the program
; params: AL = exit status
; returns: nothing
align 16
exit_program:
    movzx edi, al
    mov eax, SYSCALL_EXIT
    syscall

; display_string: prints a null-terminated string to stdout
; params: ESI = pointer to string
; returns: nothing
align 16
display_string:
    cld
    xor edx, edx
    push edi
    push esi

.loop:
    lodsb
    test al, al
    jz .counted
    inc edx
    jmp .loop

.counted:
    pop esi
    mov edi, STDOUT_FILENO
    mov eax, SYSCALL_WRITE
    syscall

    pop edi
    ret

; display_character: print a character to stdout
; params: DL = character
; returns: nothing
align 16
display_character:
    push edi
    push esi
    push eax
    push ecx

    mov [character], dl

    mov eax, SYSCALL_WRITE
    mov edi, STDOUT_FILENO
    mov esi, character
    mov edx, 1
    syscall

    pop ecx
    pop eax
    pop esi
    pop edi

    ret

; display_number: prints a number to stdout
; params: EAX = number to display
; returns: nothing
align 16
display_number:
    push ebx
    test eax, eax
    jz .zero

    push ecx
    xor ecx, ecx

.div_loop:
    xor edx, edx
    mov ebx, 10
    div ebx
    add dl, '0'
    push edx

    inc ecx
    and eax, eax
    jnz .div_loop

.print_loop:
    pop edx
    call display_character
    loop .print_loop

    pop ecx
    pop ebx
    ret

.zero:
    mov dl, '0'
    call display_character
    pop ebx
    ret

; display_block: prints a string to stdout
; params: ESI = pointer to string
; params: ECX = byte count
; returns: nothing
align 16
display_block:
    mov edi, STDOUT_FILENO
    mov edx, ecx
    mov eax, SYSCALL_WRITE
    syscall

    ret

; init_memory(): allocates the two main blocks of memory
; params: none
; returns: nothing
align 16
init_memory:
    mov edi, [memory_setting]
    shl edi, 10         ; mul 1024
    jnz .allocate
    mov edi, 0x1000000  ; linux interface defaults to 16 MiB so we'll do the same

.allocate:
    mov ecx, edi        ; ecx = memory size

    mov eax, SYSCALL_SBRK
    syscall

    xor ebx, ebx
    not ebx
    cmp eax, ebx        ; eax == -1?
    jz .failed

    mov [memory_start], eax
    add eax, ecx
    mov [memory_end], eax

    mov eax, ecx
    mov ebx, 3          ; one third of the memory size for the additional block
    xor edx, edx
    div ebx

    mov edi, eax
    mov ecx, edi        ; ecx = memory size again
    mov eax, SYSCALL_SBRK
    syscall

    xor ebx, ebx
    not ebx
    cmp eax, ebx
    jz .failed

    mov [additional_memory], eax
    add eax, ecx
    mov [additional_memory_end], eax

    ret

.failed:
    mov esi, _no_low_memory
    call display_string

    mov al, 0xFF
    call exit_program

; fatal_error(): error handler
; params: pointer to error message on top of stack
; returns: nothing
align 16
fatal_error:
    pop esi
    call display_string

    mov al, 0xFF
    call exit_program

; assembler_error(): assembler error handler
; TODO
align 16
assembler_error:
    ;call display_user_messages
    mov al, 0xFF
    call exit_program

; create(): creates a new file
; params: EDX = pointer to file name
; returns: EBX = file descriptor, CF = 0 on success
align 16
create:
    push edi
    push esi

    mov eax, SYSCALL_OPEN
    mov edi, edx
    mov esi, O_WRONLY or O_CREAT or O_TRUNC
    mov edx, S_IRUSR or S_IWUSR or S_IRGRP or S_IROTH
    syscall

    pop esi
    pop edi
    mov ebx, eax
    test ebx, 0x80000000
    jnz .error

    clc
    ret

.error:
    stc
    ret

; open(): opens a file for reading
; params: EDX = pointer to file name
; returns: EBX = file descriptor, CF = 0 on success
align 16
open:
    push edi
    push esi

    mov eax, SYSCALL_OPEN
    mov edi, edx
    mov esi, O_RDONLY
    xor edx, edx
    syscall

    pop esi
    pop edi
    mov ebx, eax
    test ebx, 0x80000000
    jnz .error

    clc
    ret

.error:
    stc
    ret

; write(): writes to a file
; params: EBX = file descriptor
; params: ECX = byte count
; params: EDX = data pointer
; returns: CF = 0 on success
align 16
write:
    push esi
    push edi
    push edx

    mov edi, ebx        ; file descriptor
    mov esi, edx        ; data
    mov edx, ecx        ; count
    mov eax, SYSCALL_WRITE
    syscall

    cmp eax, edx
    jnz .error

    pop edx
    pop edi
    pop esi

    clc
    ret

.error:
    pop edx
    pop edi
    pop esi

    stc
    ret

; read(): reads from a file
; params: EBX = file descriptor
; params: ECX = byte count
; params: EDX = data pointer
; returns: CF = 0 on success
align 16
read:
    push esi
    push edi
    push edx

    mov edi, ebx        ; file descriptor
    mov esi, edx        ; data
    mov edx, ecx        ; count
    mov eax, SYSCALL_READ
    syscall

    cmp eax, edx
    jnz .error

    pop edx
    pop edi
    pop esi

    clc
    ret

.error:
    pop edx
    pop edi
    pop esi

    stc
    ret

; close(): closes a file
; params: EBX = file descriptor
; returns: CF = 0 on success
align 16
close:
    push edi

    mov edi, ebx
    mov eax, SYSCALL_CLOSE
    syscall

    pop edi
    test eax, eax
    jnz .error

    clc
    ret

.error:
    stc
    ret

; lseek(): moves the file pointer
; params: EBX = file descriptor
; params: EDX = offset
; params: AL = origin (0 = SEEK_SET, 1 = SEEK_CUR, 2 = SEEK_END)
; returns: EAX = new position
align 16
lseek:
    push edi
    push esi

    cmp al, 2               ; SEEK_END
    jz .end

    mov edi, ebx            ; file descriptor
    mov esi, edx            ; offset
    movzx edx, al           ; origin
    inc edx                 ; the values FASM uses are only off by my own constants by one
    mov eax, SYSCALL_LSEEK
    syscall

    pop esi
    pop edi
    ret

.end:
    mov edi, ebx            ; file descriptor
    mov esi, statbuf
    mov eax, SYSCALL_FSTAT
    syscall

    mov esi, edx            ; offset
    add esi, dword [statbuf + ST_SIZE]
    mov edx, SEEK_SET
    mov eax, SYSCALL_LSEEK
    syscall

    pop esi
    pop edi

    ret

; get_environment_variable(): returns the value of an environmental variable
; TODO
align 16
get_environment_variable:
    ret

; make_timestamp(): Unix timestamp
align 16
make_timestamp:
    xor eax, eax        ; TODO
    ret