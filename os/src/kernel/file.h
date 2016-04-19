#ifndef __FILE_H
#define __FILE_H

#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <stdstream.h>

struct tailq_stream_head* stdin_buffer;
struct tailq_stream_head* stdout_buffer;
struct tailq_stream_head* stderr_buffer;

int32_t sys_write(int fd, char *ptr, size_t len);
int32_t sys_read(int fd, char *ptr, size_t len);
void file_initialise();

#endif
