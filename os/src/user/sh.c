/* MUSH: the μ shell */

#include "sh.h"

void mush() {
        char* last_line = stdmem_allocate(101);
        stdio_print("Welcome to the mu shell\n");
        while (1) {
                stdio_print("mush> ");
                char last_char = '\0';
                for (int i = 0; (i < 100) && (last_char != '\n') && (last_char != '\r'); i++) {
                        last_char = stdio_readchar();
                        last_line[i] = last_char;
                        stdio_printchar(last_char);
                }
                stdio_printchar('\n');
        }
}

void (*entry_sh)() = &mush;
