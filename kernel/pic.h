#ifndef PIC_H
#define PIC_H

#define PIC1            0x20        /* IO port for master PIC */
#define PIC2            0xA0        /* IO port for slave PIC */
#define PIC1_COMMAND    PIC1
#define PIC1_DATA       (PIC1+1)
#define PIC2_COMMAND    PIC2
#define PIC2_DATA       (PIC2+1)

#define PIC_EOI         0x20        /* End-of-interrupt command code */

void pic_remap(int offset1, int offset2);
void pic_send_eoi(unsigned char irq);
void irq_set_mask(unsigned char irq);
void irq_clear_mask(unsigned char irq);

#endif
