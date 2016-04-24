#include "cd.h"
#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <stdfile.h>
#include <syscall.h>

void cd(int32_t argc, char* argv[]) {
        filedesc_t dir;
        if (argc > 0) {
                dir = _open(argv[0], 0);

                if (dir < 0) {
                        stdio_print("Directory not found.\n");
                        stdproc_exit(-1);
                }

                _chdir(argv[0]);
                stdproc_exit(EXIT_SUCCESS);
        }

        stdproc_exit(-1);
}

proc_ptr entry_cd = &cd;
