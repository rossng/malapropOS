#include "ls.h"
#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <stdfile.h>
#include <syscall.h>

void ls(int32_t argc, char* argv[]) {
        filedesc_t dir;
        if (argc > 0) {
                dir = _open(argv[0], 0);
        } else {
                char* cwd = stdmem_allocate(sizeof(char)*100);
                _getcwd(cwd, 100);
                dir = _open(cwd, 0);
        }

        if (dir < 0) {
                stdio_print("Directory not found.\n");
                stdproc_exit(-1);
        }

        tailq_fat16_dir_head_t* files = _getdents(dir, 100);

        tailq_fat16_dir_entry_t* directory_entry;
        TAILQ_FOREACH(directory_entry, files, entries) {
                fat16_file_attr_t attributes = unpack_file_attributes(directory_entry->entry.attributes);
                if (attributes.is_subdirectory) {
                        stdio_print("d  ");
                } else {
                        stdio_print("f  ");
                }
                stdio_print(&(directory_entry->entry.filename[0]));
                if (directory_entry->entry.extension[0] != '\0') {
                        stdio_print(".");
                        stdio_print(&(directory_entry->entry.extension[0]));
                }
                stdio_print("\n");
        }

        stdproc_exit(EXIT_SUCCESS);
}

proc_ptr entry_ls = &ls;
