#include "cd.h"
#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <stdfile.h>
#include <syscall.h>

void cd(int32_t argc, char* argv[]) {
        filedesc_t root = _open("/", O_CREAT);
        tailq_fat16_dir_head_t* files = _getdents(root, 100);

        tailq_fat16_dir_entry_t* directory_entry;
        TAILQ_FOREACH(directory_entry, files, entries) {
                stdio_print(&(directory_entry->entry.filename[0]));
                stdio_print("\n");
        }

        stdproc_exit(EXIT_SUCCESS);
}

proc_ptr entry_cd = &cd;
