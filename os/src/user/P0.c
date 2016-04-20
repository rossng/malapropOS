#include "P0.h"
#include <stddef.h>
#include <stdio.h>
#include <stdproc.h>

void P0() {
        while (1) {
                for (int i = 0; i < 5; i++) {
                        stdio_print("P0\n");
                        for (int i = 0; i < 0x5000000; i++) {
                                asm volatile("nop");
                        }
                }
                stdproc_exit(EXIT_SUCCESS);
        }
}

void (*entry_P0)() = &P0;
