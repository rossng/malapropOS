#include "P1.h"
#include <stdproc.h>
#include <stddef.h>
#include <stdio.h>
#include <stdproc.h>

pid_t num_launches = 0;

void P1() {
        while (1) {
                for (int i = 0; i < 5; i++) {
                        stdio_print("P1: ");
                        stdio_printint(i);
                        stdio_print("\n");
                        for (int i = 0; i < 0x5000000; i++) {
                                asm volatile("nop");
                        }
                }
                stdproc_exit(EXIT_SUCCESS);
        }
}

void (*entry_P1)() = &P1;
