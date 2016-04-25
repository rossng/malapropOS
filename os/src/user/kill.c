#include "kill.h"
#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <stdfile.h>
#include <syscall.h>

void kill(int32_t argc, char* argv[]) {
        if (argc != 1) {
                stdproc_exit(-1);
        }

        char** end;
        long pid = stdstring_strtol(argv[0], end, 10);
        int32_t result = _kill((pid_t) pid, SIGKILL);

        if (result != 0) {
                stdproc_exit(-1);
        }

        stdproc_exit(EXIT_SUCCESS);
}

proc_ptr entry_kill = &kill;
