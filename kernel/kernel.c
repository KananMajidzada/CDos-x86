#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "pic.h"
#include "keyboard.h"
#include "console.h"
#include "ata.h"
#include "shell.h"

void kernel_main(void) {
    gdt_install();
    idt_install();
    isr_install();
    pic_remap();
    irq_install();
    keyboard_install();

    for (int i = 0; i < 16; i++) {
        pic_set_mask(i);
    }
    pic_clear_mask(1);

    __asm__ volatile ("sti");

    console_init();
    ata_init();
    console_print("READY.\n");
    shell_init();

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
