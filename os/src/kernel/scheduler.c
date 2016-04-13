#include "scheduler.h"
#include "constants.h"
#include "../user/P0.h"
#include "../user/P1.h"
#include "../device/PL011.h"

#include <stdmem.h>

pcb_t pcb[2];
pcb_t *current = NULL;
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

        //PL011_puts(UART0, "Scheduler: running\n", 19);

        // If there is a process currently executing
        if (current) {
                // Allocate a new queue entry, store the process details into it
                // and then add it to the back of the queue
                tailq_pcb_t *tailq_pcb_entry = stdmem_allocate(sizeof(tailq_pcb_t));
                stdmem_copy(&(tailq_pcb_entry->pcb.ctx), ctx, sizeof(ctx_t));
                tailq_pcb_entry->pcb.pid = current->pid;
                TAILQ_INSERT_TAIL(&head, tailq_pcb_entry, entries);
        }

        // If there is a process waiting to execute
        if (!TAILQ_EMPTY(&head)) {
                if (!current) {
                        current = stdmem_allocate(sizeof(pcb_t));
                }
                // Get a pointer to the first waiting process, copy its details
                // into current, remove from queue then store the current context into ctx
                tailq_pcb_t *p = TAILQ_FIRST(&head);
                stdmem_copy(&(current->ctx), &(p->pcb.ctx), sizeof(ctx_t));
                current->pid = p->pcb.pid;
                TAILQ_REMOVE(&head, p, entries);
                stdmem_free(p);
                stdmem_copy(ctx, &(current->ctx), sizeof(ctx_t));
        } else {
                current = NULL;
        }

        return;
}

/**
 * Terminate the current process.
 */
void scheduler_exit(ctx_t* ctx) {
        if (TAILQ_EMPTY(&head)) {
                PL011_puts(UART0, "Scheduler: cannot exit last remaining process\n", 46);
        } else if (!current) {
                PL011_puts(UART0, "Scheduler: no process currently executing\n", 42);
        } else {
                PL011_puts(UART0, "Scheduler: exiting\n", 19);
                // Schedule the next process from the queue. Do not re-add the
                // current process to the queue
                tailq_pcb_t *p = TAILQ_FIRST(&head);
                stdmem_copy(&(current->ctx), &(p->pcb.ctx), sizeof(ctx_t));
                current->pid = p->pcb.pid;
                TAILQ_REMOVE(&head, p, entries);
                stdmem_free(p);
                stdmem_copy(ctx, &(current->ctx), sizeof(ctx_t));
        }

        return;
}

/**
 * Duplicate the current process with a new PID.
 * @return the new PID if fork is successful, -1 if forking fails
 */
pid_t scheduler_fork(ctx_t* ctx) {
        if (!current) {
                PL011_puts(UART0, "Scheduler: no process currently executing\n", 42);
        } else {
                PL011_puts(UART0, "Scheduler: forking\n", 19);

                // Allocate a new entry in the scheduler queue, setting a new PID
                // and stack pointer
                tailq_pcb_t *new_process = stdmem_allocate(sizeof(tailq_pcb_t));
                new_process->pcb.pid = next_pid++;
                stdmem_copy(&(new_process->pcb.ctx), &(current->ctx), sizeof(ctx_t));
                new_process->pcb.ctx.sp = (uint32_t)(set_stack_break(1000));

                TAILQ_INSERT_TAIL(&head, new_process, entries);

                return new_process->pcb.pid;
        }

        return -1;
}

/**
 * Return the PID of the current process.
 * @return the PID of the current process, -1 if there is no current process.
 */
pid_t scheduler_getpid(ctx_t* ctx) {
        if (!current) {
                return -1;
        } else {
                return current->pid;
        }
}

/**
 * Initialise two processes, then store one process' context pointer to
 * into the provided location.
 */
void scheduler_initialise(ctx_t* ctx) {
        PL011_puts(UART0, "Scheduler: initialising\n", 24);

        TAILQ_INIT(&head);

        tailq_pcb_t *p0 = stdmem_allocate(sizeof(tailq_pcb_t));
        p0->pcb.pid = next_pid++;
        p0->pcb.ctx.cpsr = 0x50;
        p0->pcb.ctx.pc = (uint32_t)(entry_P0);
        p0->pcb.ctx.sp = (uint32_t)(set_stack_break(1000));

        TAILQ_INSERT_TAIL(&head, p0, entries);

        tailq_pcb_t *p1 = stdmem_allocate(sizeof(tailq_pcb_t));
        p1->pcb.pid = next_pid++;
        p1->pcb.ctx.cpsr = 0x50;
        p1->pcb.ctx.pc = (uint32_t)(entry_P1);
        p1->pcb.ctx.sp = (uint32_t)(set_stack_break(1000));

        TAILQ_INSERT_TAIL(&head, p1, entries);

        //PL011_puts(UART0 , "Scheduler: switching to process 0\n", 34);
        //current = TAILQ_FIRST(&head);
        //TAILQ_REMOVE(&head, current, entries);
        //memcpy(ctx, &current->ctx, sizeof(ctx_t));

        scheduler_run(ctx);

        return;
}
