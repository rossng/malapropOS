#ifndef __FILE_H
#define __FILE_H

#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <stdstream.h>
#include <stdstring.h>
#include <stdmem.h>
#include <stdfile.h>

struct tailq_stream_head* stdin_buffer;
struct tailq_stream_head* stdout_buffer;
struct tailq_stream_head* stderr_buffer;

TAILQ_HEAD(tailq_file_head_t, tailq_file_s);
struct tailq_file_head_t open_files;

typedef struct file_s {
        filedesc_t fd;
        int32_t offset;
        char* pathname;
} file_t;

typedef struct tailq_file_s {
        file_t file;
        TAILQ_ENTRY(tailq_file_s) entries;
} tailq_file_t;

int32_t sys_write(int fd, char *ptr, size_t len);
int32_t sys_read(int fd, char *ptr, size_t len);
void file_initialise();

filedesc_t sys_open(char* pathname, int32_t flags);

#endif
