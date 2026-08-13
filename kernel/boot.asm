; Multiboot header constants
MBALIGN     equ 1<<0
MEMINFO     equ 1<<1
FLAGS       equ MBALIGN | MEMINFO
MAGIC       equ 0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384      ; 16 KiB stack
stack_top:

section .text
global _start
extern kernel_main
extern bss_start
extern bss_end

_start:
    mov esp, stack_top

    ; Zero .bss manually -- GRUB does NOT guarantee this is done for us
    mov edi, bss_start
    mov ecx, bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb

    ; Push multiboot info pointer and magic for kernel_main (optional use later)
    push ebx
    push eax

    call kernel_main

    cli
.hang:
    hlt
    jmp .hang
