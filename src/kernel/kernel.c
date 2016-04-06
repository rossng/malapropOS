#include "../device/PL011.h"
#include "../device/GIC.h"
#include "../device/SP804.h"
#include "irq.h"
#include "scheduler.h"
#include "../user/syscall.h"

void kernel_handler_rst(ctx_t* ctx) {
  PL011_puts(UART0, "Starting MalapropOS\n", 20);

  scheduler_initialise(ctx);

  PL011_puts(UART0, "Configuring Timer\n", 18);

  TIMER0->Timer1Load     = 0x00100000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl     = 0x00000002; // select 32-bit   timer
  TIMER0->Timer1Ctrl    |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl    |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl    |= 0x00000080; // enable          timer

  PL011_puts(UART0, "Configuring UART\n", 17);

  UART0->IMSC           |= 0x00000010; // enable UART    (Rx) interrupt
  UART0->CR              = 0x00000301; // enable UART (Tx+Rx)

  PL011_puts(UART0, "Configuring Interrupt Controller\n", 33);

  GICC0->PMR             = 0x000000F0; // unmask all          interrupts
  GICD0->ISENABLER[ 1 ] |= 0x00001010; // enable UART (Rx) and Timer 0 interrupt (see 4-65 RealView PB Cortex-A8 Guide)
  GICC0->CTLR            = 0x00000001; // enable GIC interface
  GICD0->CTLR            = 0x00000001; // enable GIC distributor

  PL011_puts(UART0, "Enabling interrupts\n", 20);

  irq_enable();

  PL011_puts(UART0, "Relinquishing control\n", 22);

  return;
}

void kernel_handler_irq(ctx_t* ctx) {
  // Step 2: read  the interrupt identifier so we know the source.

  PL011_puts(UART0, "Handling interrupt\n", 19);
  uint32_t id = GICC0->IAR;

  // Step 4: handle the interrupt, then clear (or reset) the source.

  if ( id == GIC_SOURCE_TIMER0 ) {
    PL011_puts( UART0, "Timer\n", 6);
    scheduler_run(ctx);
    TIMER0->Timer1IntClr = 0x01;
  }

  else if ( id == GIC_SOURCE_UART0 ) {
    uint8_t x = PL011_getc( UART0 );

    PL011_puts( UART0, "Received: ", 10 );
    PL011_putc( UART0,  x );
    PL011_putc( UART0, '\n' );

    UART0->ICR = 0x10;
  }

  // Step 5: write the interrupt identifier to signal we're done.

  GICC0->EOIR = id;
}


void kernel_handler_svc( ctx_t* ctx, uint32_t id ) {
  switch( id ) {
    case 0x00 : { // yield()
      scheduler_run( ctx );
      break;
    }
    case 0x01 : { // write( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      for( int i = 0; i < n; i++ ) {
        PL011_putc( UART0, *x++ );
      }

      ctx->gpr[ 0 ] = n;
      break;
    }
    default   : { // unknown
      break;
    }
  }

  return;
}
