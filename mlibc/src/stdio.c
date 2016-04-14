#include <stdio.h>
#include <syscall.h>

void stdio_print(char* str) {
        _write(STDOUT_FILEDESC, str, stdstr_length(str));
}

void stdio_printchar(char c) {
        _write(STDOUT_FILEDESC, &c, 1);
}

void stdio_printint(int32_t n) {
        char str[10] = {0};
        stdstr_int_to_str(n, str);

        _write(STDOUT_FILEDESC, str, stdstr_length(str));
}

void stdio_file_print(filedesc_t fd, char* str) {
        _write(fd, str, stdstr_length(str));
}

/*
 * Read from STDIN until a \n character is encountered or the number of bytes read
 * is equal to nbytes. Store the result into buf, plus a terminating nul.
 * The size allocated for buf must be at least (nbytes+1).
 * @return the number of bytes read (includes newline, does not include nul)
 */
size_t stdio_readline(char* buf, size_t nbytes) {
        for (int32_t i = 0; i < nbytes; i++) {
                _read(STDIN_FILEDESC, &(buf[i]), 1);
                if (buf[i] == '\n') {
                        buf[i+1] = '\0';
                        return i+1;
                }
        }

        buf[nbytes] = '\0';
        return nbytes;
}

char stdio_readchar() {
        char c;
        _read(STDIN_FILEDESC, &c, 1);
        return c;
}
