#ifndef IRQ_H
#define IRQ_H

#include "nimbos.h"
#include <linux/irq.h>

#define IRQ_SLOT_OFFSET NIMBOS_SYSCALL_IPI_IRQ

extern struct irq_desc* (*irq_to_desc_sym)(unsigned int irq);

int slot_to_irq(int slot_num);
int irq_to_slot(int irq_num);

// int register_irq(int irq_num); 
int register_irq(int irq_num, uint32_t *vector, uint32_t *dest);
int unregister_irq(int irq_num);
int unregister_all_irqs(void);

#endif