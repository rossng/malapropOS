#include "P1.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include "../device/PL011.h"
#include "syscall.h"

void P1() {
        while (1) {
                PL011_puts(UART0, "P1r\n", 4);
                _write(1, "P1w\n", 4);
                fprintf(stdout, "P1\n");
                for (int i = 0; i < 0x5000000; i++) {
                        asm volatile("nop");
                }
        }
}

void (*entry_P1)() = &P1;
