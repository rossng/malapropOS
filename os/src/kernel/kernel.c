#include "../device/PL011.h"
#include "../device/GIC.h"
#include "../device/SP804.h"
#include "../user/syscall.h"

#include "irq.h"
#include "scheduler.h"
#include "file.h"

void kernel_handler_rst(ctx_t* ctx) {
        PL011_puts(UART0, "Starting MalapropOS\n", 20);

        scheduler_initialise(ctx);

        PL011_puts(UART0, "Configuring Timer\n", 18);

        TIMER0->Timer1Load = 0x00100001; // select period = 2^20 ticks ~= 1 sec
        TIMER0->Timer1Ctrl = 0x00000002; // select 32-bit   timer
        TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
        TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
        TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

        PL011_puts(UART0, "Configuring UART\n", 17);

        //UART0->IMSC |= 0x00000010; // enable UART    (Rx) interrupt
        UART0->CR = 0x00000301; // enable UART (Tx+Rx)

        PL011_puts(UART0, "Configuring Interrupt Controller\n", 33);

        GICC0->PMR = 0x000000F0; // unmask all interrupts
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

        if (id == GIC_SOURCE_TIMER0) {
                scheduler_run(ctx);
                TIMER0->Timer1IntClr = 0x01;
        }

        /*else if (id == GIC_SOURCE_UART0) {
                uint8_t x = PL011_getc(UART0);

                PL011_puts(UART0, "Received: ", 10);
                PL011_putc(UART0,  x);
                PL011_putc(UART0, '\n');

                UART0->ICR = 0x10;
        }*/

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
                        int fd = (int)(ctx->gpr[0]);
                        char* ptr = (char*)(ctx->gpr[1]);
                        int len = (int)(ctx->gpr[2]);

                        int32_t result = sys_write(fd, ptr, len);

                        ctx->gpr[0] = result;
                        break;
                }
                case 7 : { // waitpid
                        procevent_t status = (procevent_t)(ctx->gpr[0]);
                        pid_t pid = (pid_t)(ctx->gpr[1]);

                        event_t until_event = {status, pid};
                        scheduler_block_process(ctx, until_event);
                        // Doesn't return
                        break;
                }
                case 20 : { // getpid
                        pid_t result = scheduler_getpid(ctx);

                        ctx->gpr[0] = result;
                        break;
                }
                case 158 : { // yield
                        scheduler_run(ctx);
                        break;
                }
                case 1000 : { // non-standard exec
                        void (*function)() = (void (*)(void *)) ctx->gpr[0];

                        scheduler_exec(ctx, function);

                        break;
                }
                default : { // unknown
                        break;
                }
        }

        return;
}
