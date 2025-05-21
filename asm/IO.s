section .text
;///////////DO NOT TOUCH////////////////////////////////////////////////////////////////////////////
read:
    push rbp
    mov rbp, rsp
    call read_decimal
    mov rsp, rbp
    pop rbp
    ret

print:
    push rbp
    mov rbp, rsp
    call print_decimal
    mov rsp, rbp
    pop rbp
    ret

end:
    mov rdi, 0
    mov rax, 60
    syscall
;//////////////////////////////////////////////////////////////////////////////////////////////////

read_decimal:
    push rbp
    mov rbp, rsp
    sub rsp, 32                ; Allocate 32 bytes on stack for input buffer

    ; Read input from console
    mov rax, 0                  ; sys_read
    mov rdi, 0                  ; stdin
    lea rsi, [rsp]              ; Use stack space as buffer
    mov rdx, 32                 ; buffer size
    syscall

    ; Check if we got any input
    test rax, rax
    jz .error                   ; No input

    ; Convert ASCII to number
    xor rax, rax                ; Clear RAX for result
    xor rcx, rcx                ; Clear RCX (will be our counter/index)
    mov rsi, rsp                ; Point to start of stack buffer

.process_digit:
    mov cl, [rsi]               ; Get next character
    test cl, cl                 ; Check for null terminator
    jz .done
    cmp cl, 0x0D
    je .done
    cmp cl, 0x0A                ; Check for newline (common with sys_read)
    je .done

    ; Check if it's a valid digit ('0'-'9')
    cmp cl, '0'
    jb .error
    cmp cl, '9'
    ja .error

    sub cl, '0'                 ; Convert ASCII to digit value
    imul rax, 10
    add rax, rcx

    inc rsi                     ; Move to next character
    jmp .process_digit

.done:
    mov rsp, rbp                ; Restore stack pointer
    pop rbp
    ret

.error:
    xor rax, rax
    mov rsp, rbp                ; Restore stack pointer
    pop rbp
    ret

print_decimal:
    push rbp
    mov rbp, rsp
    sub rsp, 21                 ; Allocate 21 bytes on stack for output buffer

    ; Handle zero case specially for simpler code
    test rax, rax
    jnz .non_zero
    mov byte [rsp], '0'
    mov byte [rsp+1], 0x0A      ; Newline
    mov rsi, rsp
    mov rdx, 2
    jmp .write

.non_zero:
    lea rdi, [rsp + 20]         ; Point to end of buffer
    mov byte [rdi], 0           ; Null terminator
    mov rcx, 10                 ; Divisor

.convert_loop:
    dec rdi
    xor rdx, rdx
    div rcx
    add dl, '0'
    mov [rdi], dl
    test rax, rax
    jnz .convert_loop

    ; Calculate length: newline position - start position + 1
    mov rsi, rdi
    lea rdx, [rsp + 20]
    sub rdx, rdi
    inc rdx
    mov byte [rsp + 20], 0x0A   ; Add newline at end

.write:
    mov rax, 1                  ; sys_write
    mov rdi, 1                  ; stdout
    syscall

    mov rsp, rbp                ; Restore stack pointer
    pop rbp
    ret
