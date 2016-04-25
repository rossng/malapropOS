#include "pipeline.h"
#include <stddef.h>
#include <stdio.h>
#include <syscall.h>

void pipeline(int32_t argc, char* argv[]) {

        if (argc != 1) {
                stdio_print("Invalid arguments.\n");
                stdproc_exit(-1);
        }

        char** end;
        long out_pid = stdstring_strtol(argv[0], end, 10);

        pid_t pid = _getpid();
        stdio_print("Pipeline PID: ");
        stdio_printint(pid);
        stdio_print("\n");

        char* received_message = stdmem_allocate(sizeof(char));
        char* outward_message = stdmem_allocate(sizeof(char));

        while (1) {
                int32_t result = -1;
                do {
                        _yield();
                        result = _rmessage(received_message, 1, pid);
                } while (result < 0);

                outward_message[0] = (*received_message) + 1;

                _smessage(outward_message, 1, pid, out_pid);

                if (outward_message[0] == -1) {
                        break;
                }

        }

        stdproc_exit(EXIT_SUCCESS);
}

proc_ptr entry_pipeline = &pipeline;
