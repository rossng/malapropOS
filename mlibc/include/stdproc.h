#ifndef STDPROC_H_
#define STDPROC_H_

#include <stdtypes.h>

// Exit statuses
#define EXIT_SUCCESS 0

// Process status changes
#define PROCESS_EVENT_EXITED 1
#define PROCESS_EVENT_SUSPENDED 2       // Process goes RUNNING -> READY
#define PROCESS_EVENT_UNSUSPENDED 3     // Process goes READY -> RUNNING
#define PROCESS_EVENT_BLOCKED 4         // Process goes RUNNING -> BLOCKED
#define PROCESS_EVENT_UNBLOCKED 5       // Process goes BLOCKED -> READY
#define PROCESS_EVENT_CREATED 6         // Process starts

// Process statuses
#define PROCESS_STATUS_RUNNING 1
#define PROCESS_STATUS_READY 2
#define PROCESS_STATUS_BLOCKED 3
#define PROCESS_STATUS_TERMINATED 4

void stdproc_exit(procres_t result);
pid_t stdproc_fork(void);
pid_t stdproc_getpid(void);

#endif
