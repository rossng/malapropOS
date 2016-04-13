#include "scheduler.h"
#include "constants.h"
#include "../user/P0.h"
#include "../user/P1.h"
#include "../user/sh.h"
#include "../device/PL011.h"

#include <stdmem.h>

pcb_t pcb[2];
pcb_t *current_pcb = NULL;
pid_t next_pid = 0;
void* current_stack_break = &stack_top_usr;

TAILQ_HEAD(tailqhead, tailq_pcb_s) head;

/**
 * Like sbrk but for the stack.
 */
void* set_stack_break(int32_t increment) {
        void* old_stack_break = current_stack_break;
        current_stack_break += 1000;
        return old_stack_break;
}

/**
 * Given the current process context, switch to the other process.
 */
void scheduler_run(ctx_t* ctx) {

        do {
                // If there is a process currently executing
                if (current_pcb) {
                        // Allocate a new queue entry, store the process details into it
                        // and then add it to the back of the queue
                        tailq_pcb_t *descheduled_entry = stdmem_allocate(sizeof(tailq_pcb_t));
                        descheduled_entry->pcb = current_pcb;
                        stdmem_copy(&(descheduled_entry->pcb.ctx), ctx, sizeof(ctx_t));
                        // Mark the descheduled process as ready (but only if it isn't blocked)
                        if (descheduled_entry->pcb.status == PROCESS_STATUS_RUNNING) {
                                descheduled_entry->pcb.status = PROCESS_STATUS_READY;
                                scheduler_emit_event({PROCESS_EVENT_SUSPENDED, descheduled_entry->pcb.pid});
                        }
                        TAILQ_INSERT_TAIL(&head, descheduled_entry, entries);
                }

                // If there is a process waiting to execute
                if (!TAILQ_EMPTY(&head)) {
                        if (!current_pcb) {
                                current_pcb = stdmem_allocate(sizeof(pcb_t));
                        }
                        // Get a pointer to the first waiting process, copy its details
                        // into current, remove from queue then store the current context into ctx
                        tailq_pcb_t *p = TAILQ_FIRST(&head);
                        current_pcb = p->pcb;
                        stdmem_copy(&(current_pcb->ctx), &(p->pcb.ctx), sizeof(ctx_t));
                        TAILQ_REMOVE(&head, p, entries);
                        stdmem_free(p);
                        stdmem_copy(ctx, &(current_pcb->ctx), sizeof(ctx_t));

                } else {
                        current_pcb = NULL;
                }
        } while (current_pcb->status != PROCESS_STATUS_READY)

        current_pcb->status = PROCESS_STATUS_RUNNING;
        scheduler_emit_event({PROCESS_EVENT_UNSUSPENDED, current_pcb->pid});

        return;
}

/**
 * Block a process until a certain event.
 */
void scheduler_block_process(ctx_t* ctx, event_t until_event) {
        // Mark the process as blocked
        current_pcb->blocked_until = until_event;
        current_pcb->status = PROCESS_STATUS_BLOCKED;
        scheduler_emit_event({PROCESS_EVENT_BLOCKED, current_pcb->pid});
        // Deschedule
        scheduler_run(ctx);
}

/**
 * Terminate the current process.
 */
void scheduler_exit(ctx_t* ctx) {
        if (TAILQ_EMPTY(&head)) {
                PL011_puts(UART0, "Scheduler: cannot exit last remaining process\n", 46);
        } else if (!current_pcb) {
                PL011_puts(UART0, "Scheduler: no process currently executing\n", 42);
        } else {
                PL011_puts(UART0, "Scheduler: exiting\n", 19);
                pid_t exited_pid = current_pcb->pid;

                // Schedule the next process from the queue. Do not re-add the
                // current process to the queue
                tailq_pcb_t *p = TAILQ_FIRST(&head);
                stdmem_copy(&(current_pcb->ctx), &(p->pcb.ctx), sizeof(ctx_t));
                current_pcb->pid = p->pcb.pid;
                TAILQ_REMOVE(&head, p, entries);
                stdmem_free(p);
                stdmem_copy(ctx, &(current_pcb->ctx), sizeof(ctx_t));

                scheduler_emit_event({PROCESS_EVENT_EXITED, exited_pid});
        }

        return;
}


/**
 * Fire the actions associated with a provided process event.
 */
void scheduler_emit_event(event_t event) {
        TAILQ_FOREACH(item, &head, entries) {
                // If the process is blocked
                if (item->pcb.status == PROCESS_STATUS_BLOCKED) {
                        // And it is blocked until this event
                        if (item->pcb.blocked_until == event) {
                                // Unblock the process
                                item->pcb.status = PROCESS_STATUS_RUNNING;
                        }
                }
        }
}

/**
 * Duplicate the current process with a new PID.
 * @return the new PID if fork is successful, -1 if forking fails
 */
pid_t scheduler_fork(ctx_t* ctx) {
        if (!current_pcb) {
                PL011_puts(UART0, "Scheduler: no process currently executing\n", 42);
        } else {
                PL011_puts(UART0, "Scheduler: forking\n", 19);

                // Allocate a new entry in the scheduler queue, setting a new PID
                // and stack pointer
                tailq_pcb_t *new_process = stdmem_allocate(sizeof(tailq_pcb_t));
                new_process->pcb.pid = next_pid++;
                stdmem_copy(&(new_process->pcb.ctx), &(current_pcb->ctx), sizeof(ctx_t));
                new_process->pcb.ctx.sp = (uint32_t)(set_stack_break(1000));
                new_process->pcb.ctx.gpr[0] = 0; // The child process should receive 0 from the fork call

                TAILQ_INSERT_TAIL(&head, new_process, entries);

                scheduler_emit_event({PROCESS_EVENT_CREATED, new_process->pcb.pid});

                return new_process->pcb.pid;
        }

        return -1;
}

/**
 * Return the PID of the current process.
 * @return the PID of the current process, -1 if there is no current process.
 */
pid_t scheduler_getpid(ctx_t* ctx) {
        if (!current_pcb) {
                return -1;
        } else {
                return current_pcb->pid;
        }
}

/**
 * Create a new process from the function provided.
 * @return the pid of the new process
 */
pid_t scheduler_newprocess(ctx_t* ctx, void (*function)()) {
        tailq_pcb_t *new_proc = stdmem_allocate(sizeof(tailq_pcb_t))
        new_proc->pcb.pid = next_pid++;
        new_proc->pcb.ctx.cpsr = 0x50;
        new_proc->pcb.ctx.pc = (uint32_t)(function);
        new_proc->pcb.ctx.sp = (uint32_t)(set_stack_break(1000));
        new_proc->pcb.status = PROCESS_STATUS_RUNNING;

        TAILQ_INSERT_TAIL(&head, new_proc, entries);

        scheduler_emit_event({PROCESS_EVENT_CREATED, new_proc->pcb.pid});

        return new_proc->pcb.pid;
}

/**
 * Initialise two processes, then store one process' context pointer to
 * into the provided location.
 */
void scheduler_initialise(ctx_t* ctx) {
        TAILQ_INIT(&head);

        tailq_pcb_t *sh = stdmem_allocate(sizeof(tailq_pcb_t));
        sh->pcb.pid = next_pid++;
        sh->pcb.ctx.cpsr = 0x50;
        sh->pcb.ctx.pc = (uint32_t)(entry_sh);
        sh->pcb.ctx.sp = (uint32_t)(set_stack_break(1000));

        TAILQ_INSERT_TAIL(&head, sh, entries);

        scheduler_run(ctx);

        return;
}
