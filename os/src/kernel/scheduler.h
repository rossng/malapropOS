#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <stddef.h>
#include <stdtypes.h>
#include <datastructures/stdqueue.h>
#include <stdproc.h>

void scheduler_initialise(ctx_t* ctx);
void scheduler_run(ctx_t* ctx);
void scheduler_exit(ctx_t* ctx);
pid_t scheduler_fork(ctx_t* ctx, uint32_t sp, uint32_t fp);
int32_t scheduler_kill(ctx_t* ctx, pid_t pid);
pid_t scheduler_getpid(ctx_t* ctx);
int32_t scheduler_setpriority(ctx_t* ctx, pid_t which, int32_t priority);
pid_t scheduler_block_process(ctx_t* ctx, event_t until_event);
pid_t scheduler_has_event_occurred(ctx_t* ctx, event_t event);
pid_t scheduler_new_process(ctx_t* ctx, void (*function)());
void scheduler_emit_event(event_t event);
pid_t scheduler_exec(ctx_t* ctx, proc_ptr function, int32_t argc, char* argv[]);
tailq_pcb_head_t* scheduler_list_procs();
void copy_pcb(pcb_t* destination, pcb_t* source);
void copy_ctx(ctx_t* destination, ctx_t* source);
void copy_event(event_t* destination, event_t* source);

#endif
