#include "syscall.h"

void yield() {
  asm volatile( "svc #158     \n"  );
}

/* Initial implementations of these syscalls adapted from https://balau82.wordpress.com/2010/12/16/using-newlib-in-arm-bare-metal-programs/ */

int _close(int file) { return -1; }

int _fstat(int file, struct stat *st) {
 st->st_mode = S_IFCHR;
 return 0;
}

int _isatty(int file) { return 1; }

int _lseek(int file, int ptr, int dir) { return 0; }

int _open(const char *name, int flags, int mode) { return -1; }

int _read(int fd, char *buf, size_t nbytes) {
  int r;

  asm volatile( "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #3     \n"
                "mov %0, r0 \n"
              : "=r" (r)
              : "r" (fd), "r" (buf), "r" (nbytes)
              : "r0", "r1", "r2" );

  return r;
}

char *heap_end = 0;
caddr_t _sbrk(int incr) {
 extern char heap_low; /* Defined by the linker */
 extern char heap_top; /* Defined by the linker */
 char *prev_heap_end;

 if (heap_end == 0) {
  heap_end = &heap_low;
 }
 prev_heap_end = heap_end;

 if (heap_end + incr > &heap_top) {
  /* Heap and stack collision */
  return (caddr_t)0;
 }

 heap_end += incr;
 return (caddr_t) prev_heap_end;
 }

ssize_t _write(int fd, char *buf, size_t nbytes) {
  int r;

  asm volatile( "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #4     \n"
                "mov %0, r0 \n"
              : "=r" (r)
              : "r" (fd), "r" (buf), "r" (nbytes)
              : "r0", "r1", "r2" );

  return r;
}
