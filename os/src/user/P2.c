#include "P2.h"
#include <stddef.h>
#include <stdio.h>
#include <stdproc.h>
#include <stdfile.h>
#include <syscall.h>

void P2() {
        _open("somefile", O_CREAT);
        stdproc_exit(EXIT_SUCCESS);
}

void (*entry_P2)() = &P2;
