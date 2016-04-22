#include "P0.h"
#include <stddef.h>
#include <stdio.h>
#include <stdproc.h>

void P0() {
        while (1) {
                int i;
                for (i = 0; i < 5; i++) {
                        stdio_print("P0: ");
                        stdio_printint(i);
                        stdio_print("\n");
                        for (int j = 0; j < 0x5000000; j++) {
                                asm volatile("nop");
                        }
                }
                stdproc_exit(EXIT_SUCCESS);
        }
}

void (*entry_P0)() = &P0;
