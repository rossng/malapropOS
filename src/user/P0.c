#include "P0.h"
//#include "syscall.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

void P0() {
  char* x = "hello world, I'm P0\n";

  while( 1 ) {
    printf( "P0: hello world\n" );
    for( int i = 0; i < 0x5000000; i++ ) {
      asm volatile( "nop" );
    }
  }
}

void (*entry_P0)() = &P0;
