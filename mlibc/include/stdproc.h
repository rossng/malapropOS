#ifndef STDPROC_H_
#define STDPROC_H_

#include <stdtypes.h>
#include <datastructures/stdqueue.h>

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

// Signals
#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGABRT 6
#define SIGKILL 9
#define SIGALRM 14
#define SIGTERM 15



typedef struct {
        uint32_t cpsr, pc, gpr[13], sp, lr;
} ctx_t;

typedef struct {
        procevent_t event;
        pid_t from_process;
} event_t;

typedef struct {
        pid_t pid;
        ctx_t ctx;
        procstatus_t status;
        event_t blocked_until;
} pcb_t;

typedef struct tailq_pcb_s {
        pcb_t pcb;
        TAILQ_ENTRY(tailq_pcb_s) entries;
} tailq_pcb_t;

typedef struct tailq_pid_s {
        pid_t pid;
        TAILQ_ENTRY(tailq_pid_s) entries;
} tailq_pid_t;

typedef struct tailq_pidh_s {
        pid_t pid;
        TAILQ_ENTRY(tailq_pidh_s) entries;
} tailq_pidh_t;

typedef struct tailq_event_s {
        event_t event;
        TAILQ_ENTRY(tailq_event_s) entries;
} tailq_event_t;

TAILQ_HEAD(tailq_pcb_head_s, tailq_pcb_s);
typedef struct tailq_pcb_head_s tailq_pcb_head_t;

void stdproc_exit(procres_t result);
pid_t stdproc_fork(void);
pid_t stdproc_getpid(void);

typedef void (*proc_ptr)(int32_t, char**);

#endif
