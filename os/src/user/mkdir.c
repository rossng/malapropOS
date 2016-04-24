#include "mkdir.h"
#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <stdfile.h>
#include <syscall.h>

void mkdir(int32_t argc, char* argv[]) {
        if (argc == 1) {
                int32_t result = _mkdir(argv[0]);
                if (result != 0) {
                        stdio_print("Directory creation failed.\n");
                        stdproc_exit(-1);
                }
                stdproc_exit(EXIT_SUCCESS);
        }

        stdio_print("Invalid arguments.\n");
        stdproc_exit(-1);
}

proc_ptr entry_mkdir = &mkdir;
