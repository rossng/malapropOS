#include "../device/PL011.h"
#include "../device/GIC.h"
#include "../device/SP804.h"
#include "../user/syscall.h"
#include "../device/disk.h"

#include "irq.h"
#include "scheduler.h"
#include "file.h"
#include <stdproc.h>

void kernel_handler_rst(ctx_t* ctx) {
        PL011_puts(UART0, "Starting MalapropOS\n", 20);

        scheduler_initialise(ctx);

        file_initialise();

        PL011_puts(UART0, "Configuring Timer\n", 18);

        TIMER0->Timer1Load = 0x00100000; // select period = 2^20 ticks ~= 1 sec
        TIMER0->Timer1Ctrl = 0x00000002; // select 32-bit   timer
        TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
        TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
        TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

        PL011_puts(UART0, "Configuring UART\n", 17);

        UART0->IMSC |= 0x00000010; // enable UART    (Rx) interrupt
        UART0->CR = 0x00000301; // enable UART0 (Tx+Rx) - see PL011 manual p3-15
        UART1->CR = 0x00000301; // enable UART1 (Tx+Rx)

        // Write a '1' at the beginning of the disk
        const uint8_t ch = '1';
        disk_wr(0, &ch, 1);

        PL011_puts(UART0, "Configuring Interrupt Controller\n", 33);

        GICC0->PMR = 0x000000F0; // unmask all interrupts
        GICD0->ISENABLER[1] |= 0x00001000; // enable UART 0 interrupt
        GICD0->ISENABLER[1] |= 0x00000010; // enable Timer 0 interrupt (see 4-65 RealView PB Cortex-A8 Guide)
        GICC0->CTLR = 0x00000001; // enable GIC interface
        GICD0->CTLR = 0x00000001; // enable GIC distributor

        PL011_puts(UART0, "Enabling interrupts\n", 20);

        irq_enable();

        PL011_puts(UART0, "Relinquishing control\n", 22);

        return;
}

void kernel_handler_irq(ctx_t* ctx) {
        // Step 2: read  the interrupt identifier so we know the source.
        uint32_t id = GICC0->IAR;

        // Step 4: handle the interrupt, then clear (or reset) the source.

        switch (id) {
                case GIC_SOURCE_TIMER0: {
                        scheduler_run(ctx);
                        TIMER0->Timer1IntClr = 0x01;
                        break;
                }
                case GIC_SOURCE_UART0: {
                        char c = PL011_getc(UART0);
                        stdstream_push_char(stdin_buffer, c);
                        UART0->ICR = 0x10;
                        break;
                }
        }

        // Step 5: write the interrupt identifier to signal we're done.
        GICC0->EOIR = id;
}


void kernel_handler_svc(ctx_t* ctx, uint32_t id) {
        switch (id) {
                case 0 : { // exit
                        int status = (int)(ctx->gpr[0]);
                        scheduler_exit(ctx);
                        break;
                }
                case 2 : { // fork
                        pid_t result = scheduler_fork(ctx);

                        ctx->gpr[0] = result;
                        break;
                }
                case 3 : { // read
                        int fd = (int)(ctx->gpr[0]);
                        char* ptr = (char*)(ctx->gpr[1]);
                        int len = (int)(ctx->gpr[2]);

                        int32_t result = sys_read(fd, ptr, len);

                        ctx->gpr[0] = result;
                        break;
                }
                case 4 : { // write
                        filedesc_t fd = (filedesc_t)(ctx->gpr[0]);
                        char* ptr = (char*)(ctx->gpr[1]);
                        int32_t len = (int32_t)(ctx->gpr[2]);

                        int32_t result = sys_write(fd, ptr, len);

                        ctx->gpr[0] = result;
                        break;
                }
                case 5 : { // open
                        char* pathname = (char*)(ctx->gpr[0]);
                        int32_t flags = (int32_t)(ctx->gpr[1]);

                        filedesc_t result = sys_open(pathname, flags);

                        ctx->gpr[0] = result;
                        break;
                }
                case 6 : { // close
                        filedesc_t fd = (filedesc_t)(ctx->gpr[0]);

                        int32_t result = sys_close(fd);

                        ctx->gpr[0] = result;
                        break;
                }
                case 7 : { // waitpid
                        procevent_t status = (procevent_t)(ctx->gpr[0]);
                        pid_t pid = (pid_t)(ctx->gpr[1]);
                        int32_t options = (int32_t)(ctx->gpr[2]);

                        event_t until_event = {status, pid};

                        pid_t result;
                        if (options & WAITPID_NOHANG) {
                                result = scheduler_has_event_occurred(ctx, until_event);
                        } else {
                                result = scheduler_block_process(ctx, until_event);
                        }

                        ctx->gpr[0] = result;
                        break;
                }
                case 10 : { // unlink
                        char* pathname = (char*)(ctx->gpr[0]);

                        int32_t result = sys_unlink(pathname);

                        ctx->gpr[0] = result;
                        break;
                }
                case 19 : { // lseek
                        filedesc_t fd = (filedesc_t)(ctx->gpr[0]);
                        int32_t offset = (int32_t)(ctx->gpr[1]);
                        int32_t whence = (int32_t)(ctx->gpr[2]);

                        int32_t result = sys_lseek(fd, offset, whence);

                        ctx->gpr[0] = result;
                        break;
                }
                case 20 : { // getpid
                        pid_t result = scheduler_getpid(ctx);

                        ctx->gpr[0] = result;
                        break;
                }
                case 37 : { // kill
                        pid_t pid = (pid_t)(ctx->gpr[0]);
                        int32_t sig = (int32_t)(ctx->gpr[1]);

                        int32_t result;
                        if (sig == SIGKILL) {
                                result = scheduler_kill(ctx, pid);
                        }

                        ctx->gpr[0] = result;
                        break;
                }
                case 97 : { // setpriority
                        pid_t which = (pid_t)(ctx->gpr[0]);
                        pid_t who = (pid_t)(ctx->gpr[1]);
                        int32_t priority = (int32_t)(ctx->gpr[2]);

                        int32_t result = scheduler_setpriority(ctx, which, priority);

                        ctx->gpr[0] = result;
                        break;
                }
                case 141 : { // getdents (modified)
                        filedesc_t fd = (filedesc_t)(ctx->gpr[0]);
                        int32_t max_num = (int32_t)(ctx->gpr[1]);

                        tailq_fat16_dir_head_t* result = sys_getdents(fd, max_num);

                        ctx->gpr[0] = (int32_t)result;
                        break;
                }
                case 158 : { // yield
                        scheduler_run(ctx);
                        break;
                }
                case 1000 : { // non-standard exec
                        proc_ptr function = (proc_ptr)(ctx->gpr[0]);
                        int32_t argc = (int32_t)(ctx->gpr[1]);
                        char** argv = (char**)(ctx->gpr[2]);

                        scheduler_exec(ctx, function, argc, argv);

                        break;
                }
                default : { // unknown
                        break;
                }
        }

        return;
}
