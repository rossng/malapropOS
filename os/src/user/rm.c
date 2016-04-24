#include "rm.h"
#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <stdfile.h>
#include <syscall.h>

void rm(int32_t argc, char* argv[]) {
        if (argc == 1) {
                // Ensure the file is closed
                filedesc_t fd = _open(argv[0], 0);
                _close(fd);

                int32_t result = _unlink(argv[0]);
                if (result < 0) {
                        stdio_print("Could not find file.\n");
                        stdproc_exit(-1);
                }
                stdproc_exit(EXIT_SUCCESS);
        }

        stdio_print("Invalid arguments.\n");
        stdproc_exit(-1);
}

proc_ptr entry_rm = &rm;
