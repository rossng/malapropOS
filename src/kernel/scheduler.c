#include "scheduler.h"
#include "constants.h"
#include "../user/P0.h"
#include "../user/P1.h"
#include "../device/PL011.h"

#include "string.h"

pcb_t pcb[ 2 ];
pcb_t *current = NULL;

/**
 * Given the current process context, switch to the other process.
 */
void scheduler_run( ctx_t* ctx ) {
  if ( current == &pcb[ 0 ] ) {
    PL011_puts( UART0 , "Scheduler: switching to process 1\n", 34 );
    PL011_puts( UART0 , "ctx.pc:         ", 16 ); PL011_puti( UART0 , ctx->pc ); PL011_putc( UART0, '\n' );
    PL011_puts( UART0 , "pcb[0].ctx.pcb: ", 16 ); PL011_puti( UART0 , pcb[0].ctx.pc ); PL011_putc( UART0, '\n' );
    PL011_puts( UART0 , "pcb[1].ctx.pcb: ", 16 ); PL011_puti( UART0 , pcb[1].ctx.pc ); PL011_putc( UART0, '\n' );
    memcpy( &pcb[ 0 ].ctx, ctx, sizeof( ctx_t ) );
    PL011_puts( UART0 , "Scheduler: saved context to pcb 0\n", 34 );
    PL011_puts( UART0 , "ctx.pc:         ", 16 ); PL011_puti( UART0 , ctx->pc ); PL011_putc( UART0, '\n' );
    PL011_puts( UART0 , "pcb[0].ctx.pc:  ", 16 ); PL011_puti( UART0 , pcb[0].ctx.pc ); PL011_putc( UART0, '\n' );
    PL011_puts( UART0 , "pcb[1].ctx.pc:  ", 16 ); PL011_puti( UART0 , pcb[1].ctx.pc ); PL011_putc( UART0, '\n' );
    memcpy( ctx, &pcb[ 1 ].ctx, sizeof( ctx_t ) );
    PL011_puts( UART0 , "Scheduler: restored context from pcb 1\n", 39 );
    PL011_puts( UART0 , "ctx.pc:         ", 16 ); PL011_puti( UART0 , ctx->pc ); PL011_putc( UART0, '\n' );
    PL011_puts( UART0 , "pcb[0].ctx.pc:  ", 16 ); PL011_puti( UART0 , pcb[0].ctx.pc ); PL011_putc( UART0, '\n' );
    PL011_puts( UART0 , "pcb[1].ctx.pc:  ", 16 ); PL011_puti( UART0 , pcb[1].ctx.pc ); PL011_putc( UART0, '\n' );
    current = &pcb[ 1 ];
  }
  else if ( current == &pcb[ 1 ] ) {
    PL011_puts( UART0 , "Scheduler: switching to process 0\n", 34);
    memcpy( &pcb[ 1 ].ctx, ctx, sizeof( ctx_t ) );
    memcpy( ctx, &pcb[ 0 ].ctx, sizeof( ctx_t ) );
    current = &pcb[ 0 ];
  }

  return;
}

/**
 * Initialise two processes, then store one process' context pointer to
 * into the provided location.
 */
void scheduler_initialise( ctx_t* ctx ) {
  PL011_puts( UART0 , "Scheduler: initialising\n", 24);

  memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );
  pcb[ 0 ].pid      = 0;
  pcb[ 0 ].ctx.cpsr = 0x50;
  pcb[ 0 ].ctx.pc   = ( uint32_t )( entry_P0 );
  pcb[ 0 ].ctx.sp   = ( uint32_t )(  &tos_P0 );

  memset( &pcb[ 1 ], 0, sizeof( pcb_t ) );
  pcb[ 1 ].pid      = 1;
  pcb[ 1 ].ctx.cpsr = 0x50;
  pcb[ 1 ].ctx.pc   = ( uint32_t )( entry_P1 );
  pcb[ 1 ].ctx.sp   = ( uint32_t )(  &tos_P1 );

  PL011_puts( UART0 , "Scheduler: switching to process 0\n", 34);
  current = &pcb[ 0 ]; memcpy( ctx, &current->ctx, sizeof( ctx_t ) );

  return;
}
