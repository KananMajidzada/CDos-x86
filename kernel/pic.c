#include "pic.h"
#include "io.h"

#define PIC1        0x20
#define PIC2        0xA0
#define PIC1_CMD    PIC1
#define PIC1_DATA   (PIC1+1)
#define PIC2_CMD    PIC2
#define PIC2_DATA   (PIC2+1)

#define PIC_EOI     0x20

#define ICW1_ICW4       0x01
#define ICW1_INIT       0x10
#define ICW4_8086       0x01

void pic_remap(void)
{
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, 0x20);   /* PIC1 offset: vectors 0x20-0x27 */
    io_wait();
    outb(PIC2_DATA, 0x28);   /* PIC2 offset: vectors 0x28-0x2F */
    io_wait();

    outb(PIC1_DATA, 4);      /* tell PIC1 there's a PIC2 at IRQ2 */
    io_wait();
    outb(PIC2_DATA, 2);      /* tell PIC2 its cascade identity */
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* restore saved masks -- we'll mask everything explicitly next anyway */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void pic_set_mask(uint8_t irq_line)
{
    uint16_t port = irq_line < 8 ? PIC1_DATA : PIC2_DATA;
    uint8_t line = irq_line < 8 ? irq_line : irq_line - 8;
    uint8_t value = inb(port) | (1 << line);
    outb(port, value);
}

void pic_clear_mask(uint8_t irq_line)
{
    uint16_t port = irq_line < 8 ? PIC1_DATA : PIC2_DATA;
    uint8_t line = irq_line < 8 ? irq_line : irq_line - 8;
    uint8_t value = inb(port) & ~(1 << line);
    outb(port, value);
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}
