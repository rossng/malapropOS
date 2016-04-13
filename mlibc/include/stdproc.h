#ifndef STDPROC_H_
#define STDPROC_H_

#include <stdtypes.h>

#define EXIT_SUCCESS 0

void stdproc_exit(procres_t result);
pid_t stdproc_fork(void);
pid_t stdproc_getpid(void);

#endif
