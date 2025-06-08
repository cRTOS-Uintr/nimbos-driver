#ifndef _UINTR_H
#define _UINTR_H

#include <stdbool.h>

#define __NR_uintr_register_handler     471
#define __NR_uintr_unregister_handler   472
#define __NR_uintr_vector_fd       473
#define __NR_uintr_register_sender 474
#define __NR_uintr_unregister_sender    475

#define uintr_register_handler(handler, flags)    syscall(__NR_uintr_register_handler, handler, flags)
#define uintr_unregister_handler(flags)      syscall(__NR_uintr_unregister_handler, flags)
#define uintr_vector_fd(vector, flags)       syscall(__NR_uintr_vector_fd, vector, flags)
#define uintr_register_sender(fd, flags)     syscall(__NR_uintr_register_sender, fd, flags)
#define uintr_unregister_sender(ipi_idx, flags)   syscall(__NR_uintr_unregister_sender, ipi_idx, flags)

#define UINTR_GET_UPID_PHYS_ADDR _IOR('u', 1, uint64_t)

struct uintr_scf_descriptor {
    uint8_t opcode;
    uint64_t args[4];
    volatile uint64_t ret_val;
};

/* User Posted Interrupt Descriptor (UPID) */
struct uintr_upid {
	struct {
		uint8_t status;	/* bit 0: ON, bit 1: SN, bit 2-7: reserved */
		uint8_t reserved1;	/* Reserved */
		uint8_t nv;		/* Notification vector */
		uint8_t reserved2;	/* Reserved */
		uint32_t ndst;	/* Notification destination */
	} nc __attribute__((packed));		/* Notification control */
	uint64_t puir;		/* Posted user interrupt requests */
} __attribute__((aligned(64)));

void notify(bool is_uintr);

void __attribute__ ((interrupt)) uintr_handler(struct __uintr_frame *ui_frame,
    unsigned long long vector);

int register_sender(void);

void init_uintr_scf(struct uintr_scf_descriptor *desc, int response_fd);

#endif /* _UINTR_H */