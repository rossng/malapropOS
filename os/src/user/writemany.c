#include "writemany.h"
#include <stddef.h>
#include <stdio.h>
#include <syscall.h>

void writemany(int32_t argc, char* argv[]) {

        if (argc != 2) {
                stdio_print("Invalid arguments.\n");
                stdproc_exit(-1);
        }

        filedesc_t fd = _open(argv[0], 0);
        _lseek(fd, 0, SEEK_SET);
        if (fd < 0) {
                stdio_print("Could not find file.\n");
                stdproc_exit(-1);
        }

        char** end;
        long count = stdstring_strtol(argv[1], end, 10);

        char* buf = "123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789";

        for (int i = 0; i < count; i++) {
                _write(fd, buf, 90);
        }

        stdproc_exit(EXIT_SUCCESS);
}

proc_ptr entry_writemany = &writemany;
