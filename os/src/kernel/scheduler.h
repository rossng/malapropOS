#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <stddef.h>
#include <stdtypes.h>
#include <datastructures/stdqueue.h>
#include <stdproc.h>

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

typedef struct tailq_event_s {
        event_t event;
        TAILQ_ENTRY(tailq_event_s) entries;
} tailq_event_t;

void scheduler_initialise(ctx_t* ctx);
void scheduler_run(ctx_t* ctx);
void scheduler_exit(ctx_t* ctx);
pid_t scheduler_fork(ctx_t* ctx);
int32_t scheduler_kill(ctx_t* ctx, pid_t pid);
pid_t scheduler_getpid(ctx_t* ctx);
int32_t scheduler_setpriority(ctx_t* ctx, pid_t which, int32_t priority);
pid_t scheduler_block_process(ctx_t* ctx, event_t until_event);
pid_t scheduler_has_event_occurred(ctx_t* ctx, event_t event);
pid_t scheduler_new_process(ctx_t* ctx, void (*function)());
void scheduler_emit_event(event_t event);
pid_t scheduler_exec(ctx_t* ctx, void (*function)());
void copy_pcb(pcb_t* destination, pcb_t* source);
void copy_ctx(ctx_t* destination, ctx_t* source);
void copy_event(event_t* destination, event_t* source);

#endif
