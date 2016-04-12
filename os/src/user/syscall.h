#ifndef __SYSCALL_H
#define __SYSCALL_H

#include <stddef.h>
#include <stdtypes.h>
#include <sys/stat.h>
#include <sys/types.h>

void _yield();
void _exit(int status);
int _close(int file);
int _fstat(int file, struct stat *st);
int _isatty(int file);
int _lseek(int file, int ptr, int dir);
int _open(const char *name, int flags, int mode);
int _read(int fd, char *buf, size_t nbytes);
caddr_t _sbrk(int incr);
ssize_t _write(int fd, char *buf, size_t nbytes);

#endif
