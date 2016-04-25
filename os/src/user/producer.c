#include "producer.h"
#include <stddef.h>
#include <stdio.h>
#include <syscall.h>

void producer(int32_t argc, char* argv[]) {

        if (argc != 2) {
                stdio_print("Invalid arguments.\n");
                stdproc_exit(-1);
        }

        char** end;
        long out_pid = stdstring_strtol(argv[0], end, 10);

        pid_t pid = _getpid();

        while (1) {
                _smessage(argv[1], 1, pid, out_pid);
                for (int j = 0; j < 0x5000000; j++) {
                        asm volatile("nop");
                }
        }
        stdproc_exit(EXIT_SUCCESS);
}

proc_ptr entry_producer = &producer;
