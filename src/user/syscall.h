#ifndef __SYSCALL_H
#define __SYSCALL_H

#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

// cooperatively yield control of processor, i.e., invoke the scheduler
void yield();

// write n bytes from x to the file descriptor fd
int write( int fd, void* x, size_t n );

int _close(int file);
int _fstat(int file, struct stat *st);
int _isatty(int file);
int _lseek(int file, int ptr, int dir);
int _open(const char *name, int flags, int mode);
int _read(int file, char *ptr, int len);
caddr_t _sbrk(int incr);
int _write(int file, char *ptr, int len);

#endif
