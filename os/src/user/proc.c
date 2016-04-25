#include "proc.h"
#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <syscall.h>
#include <stdproc.h>
#include <datastructures/stdqueue.h>

void proc(int32_t argc, char* argv[]) {

        char* running_text = "RUNNING";
        char* ready_text = "READY";
        char* blocked_text = "BLOCKED";
        char* terminated_text = "TERMINATED";
        char* space = " ";
        char* newline = "\n";

        tailq_pcb_head_t* processes = _lprocs();

        tailq_pcb_t* item;
        TAILQ_FOREACH(item, processes, entries) {
                stdio_printint(item->pcb.pid);
                stdio_print(space);
                switch (item->pcb.status) {
                        case PROCESS_STATUS_RUNNING : { stdio_print(running_text); break; }
                        case PROCESS_STATUS_READY : { stdio_print(ready_text); break; }
                        case PROCESS_STATUS_BLOCKED : { stdio_print(blocked_text); break; }
                        case PROCESS_STATUS_TERMINATED : { stdio_print(terminated_text); break; }
                }
                stdio_print(newline);
        }

        stdmem_free(processes);

        stdproc_exit(EXIT_SUCCESS);
}

proc_ptr entry_proc = &proc;
