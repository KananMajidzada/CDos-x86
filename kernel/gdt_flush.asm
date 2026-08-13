global gdt_flush
gdt_flush:
    mov eax, [esp+4]   ; pointer to gdt_ptr struct, passed as arg
    lgdt [eax]

    mov ax, 0x10        ; 0x10 = our data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.flush      ; 0x08 = our code selector, far jump reloads CS
.flush:
    ret
