#ifndef __SYSCALL_H
#define __SYSCALL_H

#include <stddef.h>
#include <stdtypes.h>

void _yield();
void _exit(int32_t status);

int32_t _read(int32_t fd, char *buf, size_t nbytes);
size_t _write(int32_t fd, char *buf, size_t nbytes);

void* _sbrk(int32_t incr);

#endif
