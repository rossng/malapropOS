#include "file.h"
#include "../device/PL011.h"
#include <errno.h>
#include <unistd.h>

ssize_t sys_write(int fd, char *buf, size_t nbytes) {
        // If printing to stdout
        if (STDOUT_FILENO == fd) {
                for (int i = 0; i < nbytes; i++) {
                        PL011_putc(UART0, *buf++);
                }
                return nbytes;
        }
        // Otherwise indicate failure
        return -1;
}

ssize_t sys_read(int fd, char *buf, size_t nbytes) {
        // If reading from stdin
        if (STDIN_FILENO == fd) {
                ssize_t result = PL011_gets(UART0, buf, nbytes);
                return result;
        }
}
