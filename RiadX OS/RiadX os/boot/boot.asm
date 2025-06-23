; boot.asm - Boot sector assembly code for RiadX OS
; This is the first code that runs when the system boots

[BITS 16]                       ; 16-bit real mode
[ORG 0x7C00]                   ; Boot sector loads at 0x7C00

; Boot sector entry point
start:
    cli                         ; Disable interrupts
    xor ax, ax                 ; Clear AX register
    mov ds, ax                 ; Set data segment
    mov es, ax                 ; Set extra segment
    mov ss, ax                 ; Set stack segment
    mov sp, 0x7C00             ; Set stack pointer just below boot sector
    sti                        ; Enable interrupts

    ; Display boot message
    mov si, boot_msg
    call print_string

    ; Load second stage bootloader
    call load_stage2

    ; If we get here, something went wrong
    mov si, error_msg
    call print_string
    jmp halt

; Load second stage bootloader from disk
load_stage2:
    mov si, loading_msg
    call print_string

    ; Reset disk system
    mov ah, 0x00               ; Reset disk function
    mov dl, 0x80               ; First hard disk
    int 0x13                   ; Call BIOS disk services
    jc disk_error              ; Jump if carry flag set (error)

    ; Read sectors from disk
    mov ah, 0x02               ; Read sectors function
    mov al, 15                 ; Number of sectors to read
    mov ch, 0                  ; Cylinder number
    mov cl, 2                  ; Starting sector (sector 2)
    mov dh, 0                  ; Head number
    mov dl, 0x80               ; Drive number
    mov bx, 0x1000             ; Buffer address (0x1000:0x0000)
    mov es, bx
    mov bx, 0x0000
    int 0x13                   ; Call BIOS disk services
    jc disk_error              ; Jump if error

    ; Verify loaded code
    mov ax, 0x1000
    mov es, ax
    mov bx, 0x0000
    mov ax, [es:bx]            ; Read first word of loaded code
    cmp ax, 0x5A4D             ; Check for valid signature
    jne load_error

    ; Jump to second stage
    mov si, jumping_msg
    call print_string
    jmp 0x1000:0x0000          ; Jump to loaded code

disk_error:
    mov si, disk_error_msg
    call print_string
    jmp halt

load_error:
    mov si, load_error_msg
    call print_string
    jmp halt

; Print null-terminated string
; Input: SI = pointer to string
print_string:
    push ax
    push bx
    mov ah, 0x0E               ; BIOS teletype function
    mov bh, 0x00               ; Page number
    mov bl, 0x07               ; Text attribute (light gray on black)

.next_char:
    lodsb                      ; Load byte from [SI] into AL, increment SI
    cmp al, 0                  ; Check for null terminator
    je .done
    int 0x10                   ; Call BIOS video interrupt
    jmp .next_char

.done:
    pop bx
    pop ax
    ret

; Print newline
print_newline:
    push ax
    mov ah, 0x0E
    mov al, 0x0D               ; Carriage return
    int 0x10
    mov al, 0x0A               ; Line feed
    int 0x10
    pop ax
    ret

; Halt system
halt:
    mov si, halt_msg
    call print_string
    cli                        ; Disable interrupts
    hlt                        ; Halt processor
    jmp halt                   ; Infinite loop

; Enable A20 line using keyboard controller
enable_a20:
    push ax
    push cx

    ; Disable keyboard
    call wait_8042_command
    mov al, 0xAD
    out 0x64, al

    ; Read output port
    call wait_8042_command
    mov al, 0xD0
    out 0x64, al
    call wait_8042_data
    in al, 0x60
    push ax

    ; Write output port with A20 enabled
    call wait_8042_command
    mov al, 0xD1
    out 0x64, al
    call wait_8042_command
    pop ax
    or al, 2                   ; Set A20 bit
    out 0x60, al

    ; Enable keyboard
    call wait_8042_command
    mov al, 0xAE
    out 0x64, al

    call wait_8042_command
    pop cx
    pop ax
    ret

; Wait for keyboard controller command ready
wait_8042_command:
    push ax
.loop:
    in al, 0x64
    test al, 2
    jnz .loop
    pop ax
    ret

; Wait for keyboard controller data ready
wait_8042_data:
    push ax
.loop:
    in al, 0x64
    test al, 1
    jz .loop
    pop ax
    ret

; Detect memory using BIOS INT 15h, EAX=E820h
detect_memory:
    push ebp
    mov ebp, esp
    pusha

    mov di, memory_map         ; Destination buffer
    xor ebx, ebx              ; Clear continuation value
    mov edx, 0x534D4150       ; "SMAP" signature
    
.loop:
    mov eax, 0xE820           ; Function code
    mov ecx, 24               ; Buffer size
    int 0x15                  ; Call BIOS
    jc .error                 ; Jump if error
    
    cmp eax, 0x534D4150       ; Verify signature
    jne .error
    
    ; Store entry
    add di, 24                ; Move to next entry
    inc word [memory_map_count]
    
    test ebx, ebx             ; Check if more entries
    jnz .loop                 ; Continue if more entries
    
    clc                       ; Clear carry (success)
    jmp .done

.error:
    stc                       ; Set carry (error)

.done:
    popa
    mov esp, ebp
    pop ebp
    ret

; Switch to protected mode
enter_protected_mode:
    cli                       ; Disable interrupts

    ; Load GDT
    lgdt [gdt_descriptor]

    ; Set PE bit in CR0
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to flush pipeline and enter protected mode
    jmp 0x08:protected_mode_entry

[BITS 32]                     ; Now in 32-bit protected mode
protected_mode_entry:
    ; Set up segment registers
    mov ax, 0x10              ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000          ; Set up stack

    ; Jump to kernel
    jmp 0x100000              ; Jump to kernel at 1MB

[BITS 16]                     ; Back to 16-bit for data

; Global Descriptor Table
gdt_start:
    ; Null descriptor
    dd 0x0
    dd 0x0

    ; Code segment descriptor
    dw 0xFFFF                 ; Limit (bits 0-15)
    dw 0x0000                 ; Base (bits 0-15)
    db 0x00                   ; Base (bits 16-23)
    db 10011010b              ; Access byte
    db 11001111b              ; Granularity byte
    db 0x00                   ; Base (bits 24-31)

    ; Data segment descriptor
    dw 0xFFFF                 ; Limit (bits 0-15)
    dw 0x0000                 ; Base (bits 0-15)
    db 0x00                   ; Base (bits 16-23)
    db 10010010b              ; Access byte
    db 11001111b              ; Granularity byte
    db 0x00                   ; Base (bits 24-31)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; Size
    dd gdt_start               ; Offset

; Data section
boot_msg        db 'RiadX OS Bootloader v1.0', 0x0D, 0x0A, 0
loading_msg     db 'Loading kernel...', 0x0D, 0x0A, 0
jumping_msg     db 'Jumping to kernel...', 0x0D, 0x0A, 0
error_msg       db 'Boot failed!', 0x0D, 0x0A, 0
disk_error_msg  db 'Disk read error!', 0x0D, 0x0A, 0
load_error_msg  db 'Invalid kernel!', 0x0D, 0x0A, 0
halt_msg        db 'System halted.', 0x0D, 0x0A, 0

; Memory map storage
memory_map_count dw 0
memory_map       times 512 db 0  ; Space for memory map entries

; Padding and boot signature
times 510-($-$$) db 0             ; Pad to 510 bytes
dw 0xAA55                         ; Boot signature

; Second stage bootloader starts here (would be loaded from disk)
second_stage:
    ; This would contain the main bootloader code
    ; For now, just a placeholder
    mov si, kernel_msg
    call print_string_32
    jmp $

kernel_msg db 'Kernel loading complete!', 0

print_string_32:
    ; 32-bit string printing routine
    push eax
    push ebx
    mov ebx, 0xB8000          ; Video memory address
    mov ah, 0x07              ; Attribute (light gray on black)

.loop:
    lodsb
    cmp al, 0
    je .done
    mov [ebx], ax
    add ebx, 2
    jmp .loop

.done:
    pop ebx
    pop eax
    ret

; Fill rest of sector
times 1024-($-second_stage) db 0
