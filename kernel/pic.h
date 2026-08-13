#ifndef PIC_H
#define PIC_H

#include <stdint.h>

void pic_remap(void);
void pic_set_mask(uint8_t irq_line);
void pic_clear_mask(uint8_t irq_line);
void pic_send_eoi(uint8_t irq);

#endif
