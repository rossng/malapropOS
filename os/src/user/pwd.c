#include "pwd.h"
#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <stdfile.h>
#include <syscall.h>

void pwd(int32_t argc, char* argv[]) {
        char* cwd = stdmem_allocate(sizeof(char)*100);
        _getcwd(cwd, 100);
        stdio_print(cwd);
        stdio_print("\n");
        stdproc_exit(EXIT_SUCCESS);
}

proc_ptr entry_pwd = &pwd;
