#include "P1.h"
#include <stdproc.h>
#include <stddef.h>
#include <stdio.h>
#include <stdproc.h>

void P1() {
        pid_t new_pid;
        if (stdproc_getpid() == 1) {
                new_pid = stdproc_fork();
        }
        while (1) {
                if (stdproc_getpid() == 1) {
                        stdio_print("P1\n");
                } else {
                        stdio_print("Not P1\n");
                }
                for (int i = 0; i < 0x5000000; i++) {
                        asm volatile("nop");
                }
        }
}

void (*entry_P1)() = &P1;
