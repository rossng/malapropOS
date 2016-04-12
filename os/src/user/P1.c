#include "P1.h"
#include <stdproc.h>
#include <stddef.h>
#include <stdio.h>

void P1() {
        while (1) {
                stdio_print("P1\n");
                for (int i = 0; i < 0x5000000; i++) {
                        asm volatile("nop");
                }
        }
}

void (*entry_P1)() = &P1;
