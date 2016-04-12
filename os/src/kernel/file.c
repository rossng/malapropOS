#include "file.h"
#include "../device/PL011.h"

int32_t sys_write(int fd, char *buf, size_t nbytes) {
        // If printing to stdout
        if (STDOUT_FILEDESC == fd) {
                int32_t i;
                for (i = 0; i < nbytes; i++) {
                        PL011_putc(UART0, *buf++);
                }
                return i;
        }
        // Otherwise indicate failure
        return -1;
}

int32_t sys_read(int fd, char *buf, size_t nbytes) {
        // If reading from stdin
        if (STDIN_FILEDESC == fd) {
                int32_t result = PL011_gets(UART0, buf, nbytes);
                return result;
        }
}
