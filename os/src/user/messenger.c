#include "messenger.h"
#include <stddef.h>
#include <stdio.h>
#include <syscall.h>

void messenger(int32_t argc, char* argv[]) {
        pid_t parent_pid = _getpid();

        uint32_t fp;
        asm volatile(
                "mov %0, r11 \n"
                : "=r" (fp)
                :
                : "r0"
        );
        int32_t fork_pid = _fork(fp);

        if (fork_pid == 0) { // child
                pid_t child_pid = _getpid();
                char* received_message = stdmem_allocate(sizeof(char)*101);
                int32_t result = -1;
                do {
                        _yield();
                        result = _rmessage(received_message, 100, child_pid);
                } while (result < 0);
                received_message[100] = '\0'; // for safety, but messages should be null-terminated

                stdio_print("Received message: ");
                stdio_print(received_message);
                stdio_print("\n");

                stdproc_exit(EXIT_SUCCESS);
        } else {
                char* message = "Hi!";
                _smessage(message, 4, parent_pid, fork_pid);
                stdio_print("Sent message: ");
                stdio_print(message);
                stdio_print("\n");

                stdproc_exit(EXIT_SUCCESS);
        }
}

proc_ptr entry_messenger = &messenger;
