#ifndef __FILE_H
#define __FILE_H

#include <stddef.h>
#include <stdtypes.h>
#include <sys/stat.h>
#include <sys/types.h>

ssize_t sys_write(int fd, char *ptr, size_t len);
ssize_t sys_read(int fd, char *ptr, size_t len);

#endif
