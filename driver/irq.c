#include <linux/interrupt.h>
#include <linux/miscdevice.h>

#include "nimbos.h"
#include "irq.h"
#include "process.h"

struct irq_desc* (*irq_to_desc_sym)(unsigned int irq);

extern struct miscdevice nimbos_device;

int slot_to_irq(int slot_num) {
    return slot_num + IRQ_SLOT_OFFSET;
}

int irq_to_slot(int irq_num) {
    return irq_num - IRQ_SLOT_OFFSET;
}

static irqreturn_t irq_handler(int irq, void *dev_id)
{
    pr_debug("nimbos-driver: IRQ %d received.\n", irq);
    signal_process(irq_to_slot(irq));
    return IRQ_HANDLED;
}

// int register_irq(int irq_num) {
int register_irq(int irq_num, uint32_t *vector, uint32_t *dest) {
    int err;
    err = request_irq(irq_num, irq_handler, IRQF_SHARED, "nimbos-driver",
        &nimbos_device);
    if (err) {
        pr_err("nimbos-driver: request_irq %d returns %d\n", irq_num, err);
        free_irq(irq_num, &nimbos_device);
        return err;
    }

    struct irq_desc *desc = irq_to_desc_sym(irq_num);
    struct irq_data *data = irq_desc_get_irq_data(desc);
    // struct apic_chip_data *apicd = data->chip_data;
    // uint32_t target_vector = apicd->hw_irq_cfg.vector;
    // uint32_t target_cpu = apicd->hw_irq_cfg.dest_apicid;
    struct irq_cfg* cfg = irqd_cfg(data);
    
    uint32_t target_vector = cfg->vector;
    uint32_t target_cpu = cfg->dest_apicid;

    pr_info("nimbos-driver: irq_desc pointer: %p\n", desc);
    pr_info("nimbos-driver: irq_data pointer: %p\n", data);
    // pr_info("nimbos-driver: apic_chip_data pointer: %p\n", apicd);
    pr_info("nimbos-driver: IRQ %d registered with vector %d on CPU %d\n",
            irq_num, target_vector, target_cpu);

    pr_info("nimbos-driver: registered irq: %d\n", irq_num);
    *vector = target_vector;
    *dest = target_cpu;
    return irq_num++;
    // return (((uint64_t)target_cpu) << 32) | target_vector;
}

int unregister_irq(int irq_num) {
    free_irq(irq_num, &nimbos_device);
    return 0;
}