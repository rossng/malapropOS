/* MUSH: the μ shell */

#include "sh.h"
#include "P0.h"
#include "P1.h"
#include "syscall.h"
#include "../device/PL011.h"


void launch_process(void (*function)()) {
        pid_t child_pid = _fork();
        int32_t status;
        if (child_pid == 0) {
                // If this is the child process, exec the new process
                PL011_puts(UART0, "mush: child\n", 12);
                _exec(entry_P0);
        } else {
                // Otherwise, wait for the child to complete
                PL011_puts(UART0, "mush: parent\n", 13);
                _waitpid(PROCESS_EVENT_EXITED, child_pid);
        }
}

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
                if (stdstr_compare(last_line, "P1\r") == 0) {
                        launch_process(entry_P0);
                }
        }
}

void (*entry_sh)() = &mush;
