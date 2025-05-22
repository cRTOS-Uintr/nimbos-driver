#define _GNU_SOURCE
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <x86gprintrin.h>
#include <scf.h>
#include <unistd.h>
#include <stdio.h>
#include <uintr.h>

struct uintr_scf_descriptor *uintr_scf_desc = NULL;
int response_uipi_ind = -1;

void init_uintr_scf(struct uintr_scf_descriptor *desc, int uipi_ind) {  
    // printf("Initializing uintr_scf_desc: %p uipi_ind: %d\n", desc, uipi_ind);
    uintr_scf_desc = desc;
    response_uipi_ind = uipi_ind;
}

void __attribute__ ((interrupt)) uintr_handler(struct __uintr_frame *ui_frame,
    unsigned long long vector)
{
    poll_requests();
    // if (vector == 0) {
    //     static const char print[] = "\t-- Linux User Interrupt handler --\n";
    //     ssize_t ret = write(STDOUT_FILENO, print, sizeof(print) - 1);
    //     (void)ret;
    // }
    // else if (vector == 1) {
    //     // printf("desc_addr: %p\n", uintr_scf_desc);
    //     static const char print[] = "\t-- Linux SCF handler --\n";
    //     ssize_t ret = write(STDOUT_FILENO, print, sizeof(print) - 1);
    //     if (uintr_scf_desc == NULL) {
    //         static const char print[] = "\t-- SCF descriptor is NULL --\n";
    //         ret = write(STDOUT_FILENO, print, sizeof(print) - 1);
    //         (void)ret;
    //     }
    //     else {
    //         // printf("opcode: %d, args: %lu %p %lu\n", 
    //         //     uintr_scf_desc->opcode,
    //         //     uintr_scf_desc->args[0],
    //         //     (void *)uintr_scf_desc->args[1],
    //         //     uintr_scf_desc->args[2]);
    //         switch (uintr_scf_desc->opcode) {
    //             case IPC_OP_READ:
    //                 // Handle read operation
    //                 break;
    //             case IPC_OP_WRITE: {
    //                 int ret = do_sys_write(uintr_scf_desc->args);
    //                 uintr_scf_desc->ret_val = ret;
    //                 static const char print[] = "\t-- After do_sys_write --\n";
    //                 ret = write(STDOUT_FILENO, print, sizeof(print) - 1);
    //                 break;
    //             }
    //             case IPC_OP_OPEN:
    //                 // Handle open operation
    //                 break;
    //             case IPC_OP_CLOSE:
    //                 // Handle close operation
    //                 break;
    //             case IPC_OP_SYNCMAP:
    //                 // Handle sync map operation
    //                 break;
    //             case IPC_OP_SYNCUNMAP:
    //                 // Handle sync unmap operation
    //                 break;
    //             default:
    //                 break;
    //         }
    //     }

    //     {
    //         char buffer[40];
    //         int len = snprintf(buffer, sizeof(buffer), "\t- uipi %d -\n", response_uipi_ind);  // 先转为字符串
    //         ret = write(STDOUT_FILENO, buffer, len);                    // 直接写入
    //     }
        
    //     __sync_synchronize();
    //     static const char print2[] = "\t-- After __sync --\n";
    //     ret = write(STDOUT_FILENO, print2, sizeof(print) - 1);
    //     _senduipi(response_uipi_ind);
        
    //     static const char print3[] = "\t-- After _senduipi --\n";
    //     ret = write(STDOUT_FILENO, print3, sizeof(print) - 1);
    // }
    // else {
    //     static const char print[] = "\t-- Unknown User Interrupt vector --\n";
    //     ssize_t ret = write(STDOUT_FILENO, print, sizeof(print) - 1);
    //     (void)ret;
    // }
    _stui();
}
