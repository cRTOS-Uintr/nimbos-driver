#ifndef _UINTR_H
#define _UINTR_H

#define __NR_uintr_register_handler	449
#define __NR_uintr_unregister_handler	450
#define __NR_uintr_create_fd		451
#define __NR_uintr_register_sender	452
#define __NR_uintr_unregister_sender	453

/* For simiplicity, until glibc support is added */
#define uintr_register_handler(handler, flags)	syscall(__NR_uintr_register_handler, handler, flags)
#define uintr_unregister_handler(flags)		syscall(__NR_uintr_unregister_handler, flags)
#define uintr_create_fd(vector, flags)		syscall(__NR_uintr_create_fd, vector, flags)
#define uintr_register_sender(fd, flags)	syscall(__NR_uintr_register_sender, fd, flags)
#define uintr_unregister_sender(fd, flags)	syscall(__NR_uintr_unregister_sender, fd, flags)

#define UINTR_GET_UPID_PHYS_ADDR _IOR('u', 1, uint64_t)

struct uintr_scf_descriptor {
    uint8_t opcode;
    uint64_t args[4];
    volatile uint64_t ret_val;
};

void __attribute__ ((interrupt)) uintr_handler(struct __uintr_frame *ui_frame,
    unsigned long long vector);

void init_uintr_scf(struct uintr_scf_descriptor *desc, int response_fd);

#endif /* _UINTR_H */