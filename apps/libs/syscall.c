#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nimbos.h"
#include "scf.h"
#include <syslog.h>
#define _GNU_SOURCE

#include <x86gprintrin.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <uintr.h>

#define BUF_SIZE 1024

static int thread_count = 1;
static int uintr_fd = -1;
uint64_t upid_addr = 0;
uint64_t apic_data = 0;

void print_maps() {
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        perror("Failed to open /proc/self/maps");
        return;
    }

    char line[BUF_SIZE];
    printf("%-23s %-5s %8s %8s %5s %8s %s\n",
           "Address", "Perms", "Offset", "Dev", "Inode", "Size", "Path");

    while (fgets(line, sizeof(line), fp)) {
        uintptr_t start, end;
        sscanf(line, "%"SCNxPTR"-%"SCNxPTR" ", &start, &end);
        printf("%lx-%lx\n", start, end);
    }

    fclose(fp);
}

static void nimbos_syscall_handler(int signum);

static void *read_thread_fn(void *arg)
{
    uint64_t* args;
    struct syscall_queue_buffer *scf_buf = get_syscall_queue_buffer();
    bool is_uintr = ((long)arg) >> 16;
    uint16_t desc_index = (uint16_t)(long)arg;
    struct scf_descriptor *desc = get_syscall_request_from_index(scf_buf, desc_index);

    if (!desc) {
        return NULL;
    }


    args = desc->args;
    int fd = (int)args[0];
    char *buf = (char *)args[1];
    if ((uint64_t) buf > NIMBOS_KERNEL_BASE_VADDR) {
        buf = NIMBOS_TO_SHADOW_KERNEL_VADDR(buf);
    }
    size_t len = (size_t)args[2];
    // printf("Shadow: read fd=%d, buf=%lx, len=%lu\n", fd, (uint64_t)buf, len);
    int ret = read(fd, buf, len);
    // assert(ret == args->len);
    // int err = register_sender();
    int err = 1;
    if (err) {
        push_syscall_response(scf_buf, desc_index, ret, false);
    }
    else {
        push_syscall_response(scf_buf, desc_index, ret, is_uintr);
    }
    return NULL;
}

int do_sys_write(uint64_t *args) {
    int fd = (int)args[0];
    char *buf = (char *)args[1];
    size_t len = (size_t)args[2];
    // printf("Shadow: write fd=%d, buf=%lx, len=%lu\n", fd, (uint64_t)buf, len);
    int ret = write(fd, buf, len);
    // printf("Shadow: write ret=%d\n", ret);
    assert(ret == (int)len);
    return ret;
}

void poll_requests(bool is_uintr, bool loop)
{
    // printf("poll_requests: is_uintr=%d, loop=%d\n", is_uintr, loop);
    uint16_t desc_index;
    struct scf_descriptor desc;
    struct syscall_queue_buffer *scf_buf = get_syscall_queue_buffer();
    int nimbos_fd = *get_nimbos_fd();
    pthread_t thread; // FIXME: use global threads pool

    do {
        while (!pop_syscall_request(scf_buf, &desc_index, &desc)) {
            // printf("syscall: desc_index=%d, opcode=%d, args=%p\n", desc_index,
            // desc.opcode, desc.args);
            switch (desc.opcode) {
            case IPC_OP_READ: {
                pthread_create(&thread, NULL, read_thread_fn, (void *)(long)desc_index + (((long)is_uintr)<<16));
                break;
            }
            case IPC_OP_WRITE: {
                int ret = do_sys_write(desc.args);
                push_syscall_response(scf_buf, desc_index, ret, is_uintr);
                break;
            }
            case IPC_OP_OPEN: {
                uint64_t *args = desc.args;
                char *pathname = (char *)args[0];
                int flags = (int)args[1];
                int mode = (int)args[2];
                // printf("Shadow: open pathname=%s, flags=%x, mode=%o\n", pathname, flags, mode);
                int ret = open(pathname, flags, mode);
                push_syscall_response(scf_buf, desc_index, ret, is_uintr);
                break;
            }
            case IPC_OP_CLOSE: {
                uint64_t *args = desc.args;
                int fd = (int)args[0];
                // printf("Shadow: close fd=%d\n", fd);
                int ret = close(fd);
                push_syscall_response(scf_buf, desc_index, ret, is_uintr);
                break;
            }
            case IPC_OP_SYNCMAP: {
                // prepare args
                uint64_t *args = desc.args;
                void *vaddr = (void *)args[0];
                uint64_t len = args[1];
                uint64_t paddr = args[2];
                int prot = (int)args[3];

                // printf("Shadow: mmap vaddr=%p, len=%lu, paddr=%lx, flags=%x, fd=%d\n", vaddr, len, paddr, prot, nimbos_fd);
                void* mapped_ptr = mmap(vaddr, len, prot, MAP_SHARED | MAP_FIXED, nimbos_fd, paddr - NIMBOS_BASE_PADDR);
                int ret = 0;
                if (mapped_ptr == MAP_FAILED) {
                    ret = -1;
                }
                push_syscall_response(scf_buf, desc_index, ret, is_uintr);
                // printf("Shadow: mmap ret=%d, mapped_ptr=%p\n", ret, mapped_ptr);
                break;
            }
            case IPC_OP_SYNCUNMAP: {
                uint64_t *args = desc.args;
                void *vaddr = (void *)args[0];
                // printf("Shadow: unmap vaddr=%p, len=%lu\n", vaddr, args->len);
                int ret = munmap(vaddr, args[1]);
                // int ret = 0;
                push_syscall_response(scf_buf, desc_index, ret, is_uintr);
                // printf("Shadow: unmap ret=%d\n", ret);
                break;
            }
            case IPC_OP_FORK: {
                int pip[2];
                int err = pipe(pip);
                if (err) {
                    push_syscall_response(scf_buf, desc_index, err, is_uintr);
                    break;
                }
                int pid = fork();
                if (pid) {
                    // parent
                    close(pip[1]);

                    // wait for child process
                    int ret;
                    int n = read(pip[0], &ret, sizeof(int));
                    if (n != sizeof(int)) {
                        ret = -1;
                    }

                    // printf("child pid=%d\n", pid);
                    // printf("parent pid=%d\n", getpid());
                    
                    // push ret
                    push_syscall_response(scf_buf, desc_index, ret, is_uintr);
                    close(pip[0]);
                } else {
                    // child
                    close(pip[0]);
                    // int slot_num, response, vector, dest;
                    int response;
                    struct irq_info irq_info;
                    thread_count = 1;
                    init_uintr_scf(NULL, -1);
                    int err = ioctl(nimbos_fd, NIMBOS_SETUP_SYSCALL, &irq_info);
                    int slot_num = irq_info.slot_num;
                    uint32_t vector = irq_info.vector;
                    uint32_t dest = irq_info.dest;
                    apic_data = ((uint64_t)vector << 32) | (uint64_t)dest;

                    printf("child slot_num=%d, vector=%x, dest=%x\n", slot_num, vector, dest);

                    if (err) {
                        response = err;
                    } else {
                        set_slot_num(slot_num);
                        nimbos_setup_syscall_buffers(nimbos_fd, slot_num, &uintr_fd, &upid_addr);
                        response = slot_num;
                    }

                    int ret = write(pip[1], &response, sizeof(int));
                    assert(ret == sizeof(int));
                    close(pip[1]);
                    loop = true;
                }
                break;
            }
            case IPC_OP_STAT: {
                uint64_t *args = desc.args;
                const char* path = (char *)args[0];
                struct stat st;
                int ret = stat(path, &st);
                if (ret == 0) {
                    push_syscall_response(scf_buf, desc_index, st.st_size, is_uintr);
                } else {
                    push_syscall_response(scf_buf, desc_index, ret, is_uintr);
                }
                break;
            }
            case IPC_OP_CLONE: {
                thread_count++;
                printf("cloned, thread count: %d\n", thread_count);
                push_syscall_response(scf_buf, desc_index, 0, is_uintr);
                break;
            }
            case IPC_OP_EXIT: {
                if (thread_count == 1) {
                    ioctl(nimbos_fd, NIMBOS_EXIT, NULL);
                    push_syscall_response(scf_buf, desc_index, 0, is_uintr);
                    exit(0);
                } else {
                    thread_count--;
                    push_syscall_response(scf_buf, desc_index, 0, is_uintr);
                }
                break;
            }
            case IPC_OP_UINTR_INIT: {
                // _stui();
                syslog(LOG_INFO, "handling ICP_OP_INIT_UINTR");
                // uint64_t *args = desc.args;
                // void *upid_paddr = (void *)args[0];
                // struct uintr_scf_descriptor *uintr_scf_desc = (struct uintr_scf_descriptor *)args[1];
                // // printf("UPID addr: %p uintr_scf_desc: %p\n", upid_paddr, uintr_scf_desc);
                
                // int uipi_index;
                // // printf("upid_paddr %lx\n", upid_paddr);
                // // printf("calculated UPID addr %p\n", upid_paddr);
                // uipi_index = uintr_register_sender(upid_paddr, 1<<9);
                // if (uipi_index < 0) {
                //     printf("Sender register error\n");
                //     push_syscall_response(scf_buf, desc_index, 0);
                //     break;
                // }
                // // printf("UITTE index: %d\n", uipi_index);
                // _senduipi(uipi_index);

                // uipi_index = uintr_register_sender(upid_paddr, (1<<9) + 1);
                // if (uipi_index < 0) {
                //     printf("Scf response sender register error\n");
                //     push_syscall_response(scf_buf, desc_index, 0);
                //     break;
                // }
                // init_uintr_scf(uintr_scf_desc, uipi_index);

                
                // 3. 打印 UPID 地址
                // printf("Linux UPID address: 0x%llx\n", (unsigned long long)upid_addr);

                push_syscall_response(scf_buf, desc_index, upid_addr, is_uintr);
                loop = false;
                break;
            }
            case IPC_OP_IPI_INIT: {
                push_syscall_response(scf_buf, desc_index, apic_data, is_uintr);
                loop = false;
                break;
            }
            default:
                break;
            }
        }
    } while (loop);
    // printf("poll_requests: done\n");
}

static void nimbos_syscall_handler(int signum)
{
    // printf("received signal %d\n", signum);
    if (signum == NIMBOS_SYSCALL_SIG_NUM) {
        poll_requests(false, false);
        // notify(false);
    }
}

int nimbos_setup_syscall(uint64_t *apic_data)
{
    int fd = open(NIMBOS_DEV, O_RDWR);
    if (fd <= 0) {
        return fd;
    }
    int err = nimbos_setup_syscall_buffers(fd, 0, &uintr_fd, &upid_addr);
    if (err) {
        fprintf(stderr, "Failed to setup syscall buffers: %d\n", err);
        return err;
    }

    struct irq_info irq_info;
    err = ioctl(fd, NIMBOS_SETUP_SYSCALL, &irq_info);
    if (err) {
        fprintf(stderr, "Failed to setup syscall: %d\n", err);
        return err;
    }
    
    int slot_num = irq_info.slot_num;
    uint32_t vector = irq_info.vector;
    uint32_t dest = irq_info.dest;
    *apic_data = ((uint64_t)vector << 32) | (uint64_t)dest;

    set_slot_num(slot_num);
    // printf("syscall: slot_num=%d\n", slot_num);
    signal(NIMBOS_SYSCALL_SIG_NUM, nimbos_syscall_handler);

    // handle requests before app starting
    poll_requests(false, true);
    // notify(false);
    printf("syscall: slot_num=%d\n", slot_num);

    return fd;
}
