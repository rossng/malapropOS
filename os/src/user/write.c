#include "write.h"
#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <stdfile.h>
#include <syscall.h>

void write(int32_t argc, char* argv[]) {
        if (argc > 1) {
                filedesc_t fd = _open(argv[0], 0);
                _lseek(fd, 0, SEEK_SET);
                if (fd < 0) {
                        stdio_print("Could not find file.\n");
                        stdproc_exit(-1);
                }

                for (int i = 1; i < argc; i++) {
                        int32_t len = stdstring_length(argv[i]);
                        int32_t len_written = _write(fd, argv[i], len);
                        int32_t space_written = _write(fd, " ", 1);
                        if (len_written != len || space_written != 1) {
                                stdio_print("Ran out of space in file.\n"); // TODO: make files expandable
                                stdproc_exit(-1);
                        }
                }

                stdproc_exit(EXIT_SUCCESS);
        }

        stdio_print("Invalid arguments.\n");
        stdproc_exit(-1);
}

proc_ptr entry_write = &write;
