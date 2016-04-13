#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <stddef.h>
#include <stdtypes.h>
#include <datastructures/stdqueue.h>

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

void scheduler_initialise(ctx_t* ctx);
void scheduler_run(ctx_t* ctx);
void scheduler_exit(ctx_t* ctx);
pid_t scheduler_fork(ctx_t* ctx);
pid_t scheduler_getpid(ctx_t* ctx);
void scheduler_block_process(ctx_t* ctx, event_t until_event);
void scheduler_emit_event(event_t event);

#endif
