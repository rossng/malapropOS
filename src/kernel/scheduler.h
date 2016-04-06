#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef int pid_t;

typedef struct {
  pid_t pid;
  ctx_t ctx;
} pcb_t;

void scheduler_run( ctx_t* ctx );
void scheduler_initialise();

#endif
