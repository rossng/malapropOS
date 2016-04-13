#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <stddef.h>
#include <stdtypes.h>
#include <datastructures/stdqueue.h>

typedef struct {
        uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef int pid_t;

typedef struct {
        pid_t pid;
        ctx_t ctx;
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

#endif
