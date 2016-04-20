#ifndef __SYSCALL_H
#define __SYSCALL_H

#include <stddef.h>
#include <stdtypes.h>

#define WAITPID_NOHANG 1

void _yield(void);
void _exit(procres_t result);
pid_t _fork(void);
pid_t _getpid(void);
pid_t _waitpid(procevent_t event, pid_t pid, int32_t options);
void _exec(void (*function)());

int32_t _read(int32_t fd, char *buf, size_t nbytes);
size_t _write(int32_t fd, char *buf, size_t nbytes);

void* _sbrk(int32_t incr);

#endif
