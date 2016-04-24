#include "touch.h"
#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <stdfile.h>
#include <syscall.h>

void touch(int32_t argc, char* argv[]) {
        if (argc == 1) {
                int32_t result = _open(argv[0], O_CREAT);
                if (result < 0) {
                        stdio_print("Could not find or create file.\n");
                        stdproc_exit(-1);
                }
                stdproc_exit(EXIT_SUCCESS);
        }

        stdio_print("Invalid arguments.\n");
        stdproc_exit(-1);
}

proc_ptr entry_touch = &touch;
