#ifndef __FILE_H
#define __FILE_H

#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>

int32_t sys_write(int fd, char *ptr, size_t len);
int32_t sys_read(int fd, char *ptr, size_t len);

#endif
