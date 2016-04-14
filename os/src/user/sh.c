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

void get_line(char* buf, size_t nbytes) {
        buf[0] = '\0';
        char last_char = '\0';
        int32_t cursor_index = 0;
        int32_t end_of_str = 0;
        while ((cursor_index < nbytes) && (last_char != '\n') && (last_char != '\r')) {
                last_char = stdio_readchar();
                // Backspace has some weird legacy mapping: http://www.ibb.net/%7Eanne/keyboard/keyboard.html
                if (last_char == '\b' || last_char == 0x7f) {
                        if (cursor_index > 0) {
                                stdio_print("\b \b"); // Go back, overwrite the character with a space, go back again
                                cursor_index--;
                                end_of_str--;
                        }
                } else if (last_char == '\n' || last_char == '\r') {
                        cursor_index = end_of_str;
                        buf[cursor_index] = '\r';
                        cursor_index++;
                        stdio_print("\r");
                } else if (last_char == 27) { // ESC
                        char next = stdio_readchar();
                        if (next != '[') {
                                stdio_print("^ESC");
                                stdio_printchar(next);
                                end_of_str += 4;
                                cursor_index += 4;
                        } else if (stdio_readchar() == 'D') { // Left arrow
                                if (cursor_index > 0) {
                                        stdio_printchar('\b');
                                        cursor_index--;
                                }
                        } else if (stdio_readchar() == 'C') { // Right arrow
                                if (cursor_index < end_of_str) {
                                        cursor_index++;
                                }
                        }
                } else {
                        char prev = buf[cursor_index];
                        char current;
                        buf[cursor_index] = last_char;
                        stdio_printchar(buf[cursor_index]);
                        end_of_str++;
                        cursor_index++;
                        for (int fixup_index = cursor_index; fixup_index < end_of_str; fixup_index++) {
                                stdio_printchar(prev);
                                current = buf[fixup_index];
                                buf[fixup_index] = prev;
                                prev = current;
                        }
                        for (int backspaces_needed = end_of_str-cursor_index; backspaces_needed > 0; backspaces_needed--) {
                                stdio_printchar('\b');
                        }
                }
        }
        buf[cursor_index] = '\0';
}

void mush() {
        char* last_line = stdmem_allocate(101);
        stdio_print("Welcome to the mu shell\n");
        while (1) {
                stdio_print("mush> ");
                get_line(last_line, 100);
                stdio_printchar('\n');
                if (stdstr_compare(last_line, "exit\r") == 0) {
                        _exit(EXIT_SUCCESS);
                } else if (stdstr_compare(last_line, "P1\r") == 0) {
                        launch_process(entry_P0);
                }
        }
}

void (*entry_sh)() = &mush;
