#include "cat.h"
#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <stdfile.h>
#include <syscall.h>

void cat(int32_t argc, char* argv[]) {
        if (argc == 1) {
                filedesc_t fd = _open(argv[0], 0);
                _lseek(fd, 0, SEEK_SET);
                fat16_dir_entry_t* file_entry = stdmem_allocate(sizeof(fat16_dir_entry_t));
                if (_fstat(fd, file_entry) != 0) {
                        stdio_print("File not found.\n");
                        stdproc_exit(-1);
                }

                fat16_file_attr_t attr = unpack_file_attributes(file_entry->attributes);

                if (attr.is_subdirectory) {
                        stdio_print("Cannot cat a directory.\n");
                        stdproc_exit(-1);
                }

                char* buf = stdmem_allocate(file_entry->file_size_bytes + 1);

                int32_t bytes_read = _read(fd, buf, file_entry->file_size_bytes);
                buf[bytes_read] = '\0';

                stdio_print(buf);
                stdio_print("\n");
                stdproc_exit(EXIT_SUCCESS);
        }

        stdio_print("Invalid arguments.\n");
        stdproc_exit(-1);
}

proc_ptr entry_cat = &cat;
