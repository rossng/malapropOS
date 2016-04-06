#include "P1.h"
#include "syscall.h"

void P1() {
  char* x = "hello world, I'm P1\n";

  while( 1 ) {
    write( 0, x, 20 );

    for( int i = 0; i < 0x5000000; i++ ) {
      asm volatile( "nop" );
    }
  }
}

void (*entry_P1)() = &P1;
