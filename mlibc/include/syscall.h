#ifndef __SYSCALL_H
#define __SYSCALL_H

#include <stddef.h>
#include <stdtypes.h>
#include <stdfile.h>

#define WAITPID_NOHANG 1

void _yield(void);
void _exit(procres_t result);
pid_t _fork(void);
pid_t _getpid(void);
pid_t _waitpid(procevent_t event, pid_t pid, int32_t options);
void _exec(proc_ptr function, int32_t argc, char* argv[]);
int32_t _kill(pid_t pid, int32_t sig);
int32_t _setpriority(pid_t which, pid_t who, int32_t priority);

int32_t _read(int32_t fd, char* buf, size_t nbytes);
size_t _write(int32_t fd, char* buf, size_t nbytes);

filedesc_t _open(char* pathname, int32_t flags);
int32_t _close(filedesc_t fd);
int32_t _unlink(char* pathname);
int32_t _lseek(filedesc_t fd, int32_t offset, int32_t whence);
tailq_fat16_dir_head_t* _getdents(filedesc_t fd, int32_t max_num);

void* _sbrk(int32_t incr);

#endif
