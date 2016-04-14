#ifndef SH_H_
#define SH_H_

#include <stddef.h>
#include <stdio.h>
#include <stdproc.h>
#include <stdmem.h>

extern void (*entry_sh)();

typedef struct {
        char* prompt;
        size_t prompt_bytes;
        char* contents;
        size_t contents_bytes;
        int32_t cursor_position;
} shell_line_t;

#endif
