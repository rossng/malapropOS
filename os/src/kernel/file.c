#include "file.h"
#include "../device/PL011.h"

struct tailq_stream_head* stdin_buffer;
struct tailq_stream_head* stdout_buffer;
struct tailq_stream_head* stderr_buffer;

struct tailq_file_head_t open_files;


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
        TAILQ_INIT(&open_files);
}

int next_filedesc = 3;
filedesc_t get_next_filedesc() {
        filedesc_t current_filedesc = next_filedesc;
        next_filedesc++;
        return current_filedesc;
}

file_t* find_open_file(char* pathname) {
        tailq_file_t* file;
        TAILQ_FOREACH(file, &open_files, entries) {
                if (stdstring_compare(file->file.pathname, pathname) == 0) {
                        return &(file->file);
                }
        }
        return NULL;
}

file_t* create_new_file(char* pathname) {
        tailq_file_t* new_file = stdmem_allocate(sizeof(tailq_file_t));
        char* copied_pathname = stdmem_allocate(sizeof(char)*(stdstring_length(pathname) + 1));
        stdmem_copy(copied_pathname, pathname, sizeof(char)*(stdstring_length(pathname) + 1));
        new_file->file.fd = get_next_filedesc();
        new_file->file.offset = 0;
        new_file->file.pathname = copied_pathname;

        TAILQ_INSERT_TAIL(&open_files, new_file, entries);

        return &(new_file->file);
}

filedesc_t sys_open(char* pathname, int32_t flags) {
        file_t* result = find_open_file(pathname);
        if (result == NULL) {
                if (flags & O_CREAT) {
                        result = create_new_file(pathname);
                } else {
                        return -1;
                }
        }

        if (flags & O_APPEND) { // Move offset to end of file
                result->offset = 0; // TODO
        } else { // Move offset to beginning of file
                result->offset = 0;
        }

        return result->fd;
}
