#ifndef _NIMBOS_H
#define _NIMBOS_H

#define NIMBOS_DEV "/dev/nimbos"

#define NIMBOS_SYSCALL_SIG_NUM 44

#define NIMBOS_SETUP_SYSCALL _IO(0, 0)
#define NIMBOS_EXIT _IO(0, 1)
#define NIMBOS_NOTIFY _IO(1, 2)

#include <stdint.h>

int nimbos_setup_syscall(uint64_t *apic_data);

struct irq_info {
    int slot_num;
    uint32_t vector;
    uint32_t dest;
};

#endif /* !_NIMBOS_H */
