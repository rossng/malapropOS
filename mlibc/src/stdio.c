#include <stdio.h>
#include <syscall.h>

void stdio_print(char* str) {
        _write(STDIN_FILEDESC, str, stdstr_length(str));
}

void stdio_file_print(filedesc_t fd, char* str) {
        _write(fd, str, stdstr_length(str));
}
