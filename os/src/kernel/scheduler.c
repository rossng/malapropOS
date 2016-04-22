#include "scheduler.h"
#include "constants.h"
#include "../user/P0.h"
#include "../user/P1.h"
#include "../user/sh.h"
#include "../device/PL011.h"

#include <stdmem.h>

pid_t current_pid = NULL;
pid_t next_pid = 1;
void* current_stack_break = &stack_top_usr;

TAILQ_HEAD(tailq_pcb_head_t, tailq_pcb_s);
TAILQ_HEAD(tailq_pid_head_t, tailq_pid_s);
TAILQ_HEAD(tailq_pidh_head_t, tailq_pidh_s);
TAILQ_HEAD(tailq_event_head_t, tailq_event_s) events_head;

struct tailq_pcb_head_t process_list;
struct tailq_pid_head_t low_priority_queue_head;
struct tailq_pidh_head_t high_priority_queue_head;

/**
 * Like sbrk but for the stack.
 */
void* set_stack_break(int32_t increment) {
        void* old_stack_break = current_stack_break;
        current_stack_break -= increment;
        return old_stack_break;
}

pid_t get_new_pid() {
        pid_t pid = next_pid;
        next_pid++;
        return pid;
}

/**
 * Find a process in the process list by its PID
 */
pcb_t* find_process(pid_t pid) {
        tailq_pcb_t* item;
        TAILQ_FOREACH(item, &process_list, entries) {
                if (item->pcb.pid == pid) {
                        return &(item->pcb);
                }
        }
        return NULL;
}

/**
 * Save the details of the current process to the process list.
 */
void save_current_process(ctx_t* ctx) {
        // Retrieve the saved state of the current process
        pcb_t* current_process = find_process(current_pid);
        // Update it with the current context
        copy_ctx(&(current_process->ctx), ctx);
}

/**
 * Schedule a process by its PID.
 */
void run_process(ctx_t* ctx, pid_t pid) {
        // Retrieve the saved details of the specified process
        pcb_t* to_schedule = find_process(pid);
        // Copy them into the context
        copy_ctx(ctx, &(to_schedule->ctx));
        // And update the current pid
        current_pid = pid;
}

/**
 * Remove a PID from the specified priority queue
 */
void remove_pid(pid_t pid, int32_t priority) {
        switch (priority) {
                case 0 : {
                        tailq_pidh_t* itemh;
                        TAILQ_FOREACH(itemh, &high_priority_queue_head, entries) {
                                if (itemh->pid == pid) {
                                        TAILQ_REMOVE(&high_priority_queue_head, itemh, entries);
                                }
                        }
                }
                case 1 : {
                        tailq_pid_t* item;
                        TAILQ_FOREACH(item, &low_priority_queue_head, entries) {
                                if (item->pid == pid) {
                                        TAILQ_REMOVE(&low_priority_queue_head, item, entries);
                                }
                        }
                }
        }
}

/**
 * Add a PID to the specified priority queue
 */
void add_pid(pid_t pid, int32_t priority) {

        switch (priority) {
                case 0 : {
                        tailq_pidh_t *new_pidh = stdmem_allocate(sizeof(tailq_pidh_t));
                        new_pidh->pid = pid;
                        TAILQ_INSERT_TAIL(&high_priority_queue_head, new_pidh, entries);
                }
                case 1 : {
                        tailq_pid_t *new_pid = stdmem_allocate(sizeof(tailq_pid_t));
                        new_pid->pid = pid;
                        TAILQ_INSERT_TAIL(&low_priority_queue_head, new_pid, entries);
                }
        }

}

/**
 * Set the priority of a process
 */
void set_priority(pid_t pid, int32_t priority) {
        remove_pid(pid, priority);
        add_pid(pid, priority);
}

/**
 * Get the PID of the next process in the specified priority queue
 * @return -1 if the high priority queue is empty
 */
pid_t get_next_process(int32_t priority) {

        switch (priority) {
                case 0 : {
                        tailq_pidh_t* next_pidh_item;

                        // Move the first item in the queue to the back and return its PID
                        if (TAILQ_EMPTY(&high_priority_queue_head)) {
                                return -1;
                        }
                        next_pidh_item = TAILQ_FIRST(&high_priority_queue_head);
                        TAILQ_REMOVE(&high_priority_queue_head, next_pidh_item, entries);
                        TAILQ_INSERT_TAIL(&high_priority_queue_head, next_pidh_item, entries);
                        return next_pidh_item->pid;
                }
                case 1 : {
                        tailq_pid_t* next_pid_item;

                        // Move the first item in the queue to the back and return its PID
                        if (TAILQ_EMPTY(&low_priority_queue_head)) {
                                return -1;
                        }
                        next_pid_item = TAILQ_FIRST(&low_priority_queue_head);
                        TAILQ_REMOVE(&low_priority_queue_head, next_pid_item, entries);
                        TAILQ_INSERT_TAIL(&low_priority_queue_head, next_pid_item, entries);
                        return next_pid_item->pid;
                }
                default : {
                        return -1;
                }

        }
}

/**
 * Given the current process context, switch to the other process.
 */
void scheduler_run(ctx_t* ctx) {
        pid_t next_proc_pid = get_next_process(0);

        if (next_proc_pid == -1) {
                next_proc_pid = get_next_process(1);
        }

        save_current_process(ctx);
        run_process(ctx, next_proc_pid);
        return;
}

/**
 * Block a process until a certain event.
 */
pid_t scheduler_block_process(ctx_t* ctx, event_t until_event) {
        // Save the current process state and mark it as blocked
        save_current_process(ctx);
        pcb_t* current_process = find_process(current_pid);
        current_process->blocked_until = until_event;
        current_process->status = PROCESS_STATUS_BLOCKED;

        // Remove the current process from all priority queues
        remove_pid(current_pid, 0);
        remove_pid(current_pid, 1);

        scheduler_emit_event((event_t){PROCESS_EVENT_BLOCKED, current_pid});

        // Find another process to schedule
        scheduler_run(ctx);

        return until_event.from_process;
}

/**
 * Check the record to see if an event has occurred.
 */
pid_t scheduler_has_event_occurred(ctx_t* ctx, event_t event) {
        tailq_event_t* item;
        TAILQ_FOREACH(item, &events_head, entries) {
                // If the process is blocked
                if (item->event.event == event.event && item->event.from_process == event.from_process) {
                        return event.from_process;
                }
        }
        return 0;
}

/**
 * Terminate the current process.
 */
void scheduler_exit(ctx_t* ctx) {
        // Save the current pid
        pid_t exited_pid = current_pid;

        // Remove the current process from all priority queues
        remove_pid(current_pid, 0);
        remove_pid(current_pid, 1);

        // And ask the scheduler to run something else
        scheduler_run(ctx);

        // If the new current pid is different, we have exited and can mark the process
        // terminated. If not, we have failed to exit and print an error message
        if (current_pid != exited_pid) {
                pcb_t* exited_process = find_process(exited_pid);
                exited_process->status = PROCESS_STATUS_TERMINATED;
                scheduler_emit_event((event_t){PROCESS_EVENT_EXITED, exited_pid});
        } else {
                PL011_puts(UART0, "Scheduler: cannot exit last remaining process\n", 46);
        }

        return;
}

/**
 * Kill the specified process.
 */
int32_t scheduler_kill(ctx_t* ctx, pid_t pid) {
        // Remove the specified process from all priority queues
        remove_pid(pid, 0);
        remove_pid(pid, 1);

        // And ask the scheduler to run something else
        scheduler_run(ctx);

        // Notify the world
        scheduler_emit_event((event_t){PROCESS_EVENT_EXITED, pid});

        return 0;
}


/**
 * Fire the actions associated with a provided process event.
 */
void scheduler_emit_event(event_t event) {
        // Record this event in the list of events
        if (event.event != PROCESS_EVENT_SUSPENDED && event.event != PROCESS_EVENT_UNSUSPENDED) {
                tailq_event_t *ev = stdmem_allocate(sizeof(tailq_event_t));
                ev->event.event = event.event;
                ev->event.from_process = event.from_process;
                TAILQ_INSERT_TAIL(&events_head, ev, entries);
        }

        tailq_pcb_t* item;
        TAILQ_FOREACH(item, &process_list, entries) {
                // If the process is blocked
                if (item->pcb.status == PROCESS_STATUS_BLOCKED) {
                        // And it is blocked until this event
                        if (item->pcb.blocked_until.event == event.event && item->pcb.blocked_until.from_process == event.from_process) {
                                // Unblock the process
                                item->pcb.status = PROCESS_STATUS_RUNNING;
                                // Set the return value of _waitpid to the pid of the event process
                                item->pcb.ctx.gpr[0] = event.from_process;
                                // Re-add the process to a priority queue (low for simplicity)
                                set_priority(item->pcb.pid, 1);
                        }
                }
        }
}

/**
 * Set the priority of the specified process.
 * 0 = high, 1 = low
 * @return 0 = success, -1 = error
 */
int32_t scheduler_setpriority(ctx_t* ctx, pid_t pid, int32_t priority) {
        set_priority(pid, priority);
        return 0;
}

/**
 * Duplicate the current process with a new PID.
 * @return the new PID if fork is successful, -1 if forking fails
 */
pid_t scheduler_fork(ctx_t* ctx) {
        if (current_pid < 1) {
                PL011_puts(UART0, "Scheduler: no process currently executing\n", 42);
        } else {
                // Allocate a new entry in the process list, setting a new PID
                // and stack pointer
                tailq_pcb_t *new_process = stdmem_allocate(sizeof(tailq_pcb_t));
                pid_t new_pid = get_new_pid();
                pcb_t* process_to_copy = find_process(current_pid);
                copy_pcb(&(new_process->pcb), process_to_copy);
                copy_ctx(&(new_process->pcb.ctx), ctx);
                new_process->pcb.pid = new_pid;
                new_process->pcb.status = PROCESS_STATUS_READY;
                new_process->pcb.ctx.gpr[0] = 0; // The child process should receive 0 from the fork call

                TAILQ_INSERT_TAIL(&process_list, new_process, entries);

                // Now queue the new process
                set_priority(new_pid, 1);
                scheduler_emit_event((event_t){PROCESS_EVENT_CREATED, new_process->pcb.pid});


                return new_process->pcb.pid;
        }

        return -1;
}

/**
 * Return the PID of the current process.
 * @return the PID of the current process, -1 if there is no current process.
 */
pid_t scheduler_getpid(ctx_t* ctx) {
        return current_pid;
}

/**
 * Create a new process from the function provided.
 * @return the pid of the new process
 */
pid_t scheduler_exec(ctx_t* ctx, void (*function)()) {
        void* new_sp = set_stack_break(2000);
        ctx->pc = (uint32_t)(function);
        ctx->gpr[11] = (uint32_t) new_sp; // Frame pointer
        ctx->sp = (uint32_t) new_sp;
        //ctx->sp = (uint32_t)(set_stack_break(1000));
        save_current_process(ctx);
        pcb_t* current_process = find_process(current_pid);
        current_process->status = PROCESS_STATUS_RUNNING;

        return current_pid;
}

/**
 * Initialise two processes, then store one process' context pointer to
 * into the provided location.
 */
void scheduler_initialise(ctx_t* ctx) {
        TAILQ_INIT(&low_priority_queue_head);
        TAILQ_INIT(&high_priority_queue_head);
        TAILQ_INIT(&events_head);
        TAILQ_INIT(&process_list);

        pid_t new_pid = get_new_pid();
        void* new_sp = set_stack_break(2000);

        tailq_pcb_t *sh = stdmem_allocate(sizeof(tailq_pcb_t));
        sh->pcb.pid = new_pid;
        sh->pcb.ctx.cpsr = 0x50;
        sh->pcb.ctx.pc = (uint32_t) entry_sh;
        sh->pcb.ctx.sp = (uint32_t) new_sp;
        sh->pcb.status = PROCESS_STATUS_RUNNING;
        TAILQ_INSERT_TAIL(&process_list, sh, entries);

        tailq_pid_t *sh_pid = stdmem_allocate(sizeof(tailq_pid_t));
        sh_pid->pid = new_pid;
        TAILQ_INSERT_TAIL(&low_priority_queue_head, sh_pid, entries);

        run_process(ctx, new_pid);

        return;
}

void copy_pcb(pcb_t* destination, pcb_t* source) {
        destination->pid = source->pid;
        destination->status = source->status;
        copy_ctx(&destination->ctx, &source->ctx);
        copy_event(&destination->blocked_until, &source->blocked_until);
}

void copy_ctx(ctx_t* destination, ctx_t* source) {
        destination->cpsr = source->cpsr;
        stdmem_copy(&destination->gpr, &source->gpr, sizeof(uint32_t)*13);
        destination->lr = source->lr;
        destination->pc = source->pc;
        destination->sp = source->sp;
}

void copy_event(event_t* destination, event_t* source) {
        destination->event = source->event;
        destination->from_process = source->from_process;
}


char* running = "RUNNING";
char* ready = "READY";
char* blocked = "BLOCKED";
char* terminated = "TERMINATED";

char debug_result[200];

char* debug_list_processes() {
        int32_t result_index = 0;
        result_index = stdstring_append(debug_result, result_index, "Processes: ");

        tailq_pcb_t* item;
        char* tmp = stdmem_allocate(sizeof(char)*15);
        TAILQ_FOREACH(item, &process_list, entries) {
                stdstring_int_to_str(item->pcb.pid, tmp);
                result_index = stdstring_append(debug_result, result_index, "{");
                result_index = stdstring_append(debug_result, result_index, tmp);
                result_index = stdstring_append(debug_result, result_index, ", ");
                switch (item->pcb.status) {
                        case PROCESS_STATUS_RUNNING : { result_index = stdstring_append(debug_result, result_index, running); break; }
                        case PROCESS_STATUS_READY : { result_index = stdstring_append(debug_result, result_index, ready); break; }
                        case PROCESS_STATUS_BLOCKED : { result_index = stdstring_append(debug_result, result_index, blocked); break; }
                        case PROCESS_STATUS_TERMINATED : { result_index = stdstring_append(debug_result, result_index, terminated); break; }
                }

                result_index = stdstring_append(debug_result, result_index, "} ");
        }

        return debug_result;
}

char* debug_priority(int32_t priority) {
        int32_t result_index = 0;
        result_index = stdstring_append(debug_result, result_index, "Priority queue: ");

        if (priority == 0) {
                tailq_pidh_t* item;
                char* tmp = stdmem_allocate(sizeof(char)*15);
                TAILQ_FOREACH(item, &high_priority_queue_head, entries) {
                        stdstring_int_to_str(item->pid, tmp);
                        result_index = stdstring_append(debug_result, result_index, "{");
                        result_index = stdstring_append(debug_result, result_index, tmp);
                        result_index = stdstring_append(debug_result, result_index, "} ");
                }
                return debug_result;

        } else if (priority == 1) {
                tailq_pid_t* item;
                char* tmp = stdmem_allocate(sizeof(char)*15);
                TAILQ_FOREACH(item, &low_priority_queue_head, entries) {
                        stdstring_int_to_str(item->pid, tmp);
                        result_index = stdstring_append(debug_result, result_index, "{");
                        result_index = stdstring_append(debug_result, result_index, tmp);
                        result_index = stdstring_append(debug_result, result_index, "} ");
                }
                return debug_result;

        } else {
                return "";
        }
}
