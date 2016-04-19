#include "file.h"
#include "../device/PL011.h"

struct tailq_stream_head* stdin_buffer;
struct tailq_stream_head* stdout_buffer;
struct tailq_stream_head* stderr_buffer;

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
                int32_t i = 0;
                while (i < nbytes) {
                        buf[i] = stdstream_pop_char(stdin_buffer);
                        i++;
                }
                return i;
        } else {
                return -1;
        }
}

void file_initialise() {
        stdin_buffer = stdstream_initialise_buffer();
        stdout_buffer = stdstream_initialise_buffer();
        stderr_buffer = stdstream_initialise_buffer();
}
