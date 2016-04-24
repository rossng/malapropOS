#include "P2.h"
#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <stdproc.h>
#include <stdfile.h>
#include <syscall.h>

void P2() {
        filedesc_t root = _open("/", O_CREAT);
        tailq_fat16_dir_head_t* files = _getdents(root, 100);

        tailq_fat16_dir_entry_t* directory_entry;
        TAILQ_FOREACH(directory_entry, files, entries) {
                stdio_print(&(directory_entry->entry.filename[0]));
                stdio_print("\n");
        }

        stdproc_exit(EXIT_SUCCESS);
}

void (*entry_P2)() = &P2;
