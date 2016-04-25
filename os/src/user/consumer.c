#include "consumer.h"
#include <stddef.h>
#include <stdio.h>
#include <syscall.h>

void consumer(int32_t argc, char* argv[]) {

        pid_t pid = _getpid();
        stdio_print("Consumer PID: ");
        stdio_printint(pid);
        stdio_print("\n");

        char* received_message = stdmem_allocate(sizeof(char));

        while (1) {
                int32_t result = -1;
                do {
                        _yield();
                        result = _rmessage(received_message, 1, pid);
                } while (result < 0);

                stdio_print("Consumer: received ");
                stdio_printchar(*received_message);
                stdio_print("\n");

                if (*received_message == -1) {
                        break;
                }

        }

        stdproc_exit(EXIT_SUCCESS);
}

proc_ptr entry_consumer = &consumer;
