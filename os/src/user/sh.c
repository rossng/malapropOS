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
                _exec(entry_P0);
        } else {
                // Otherwise, wait for the child to complete
                _waitpid(PROCESS_EVENT_EXITED, child_pid);
        }
}

shell_line_t make_line(char* prompt, size_t prompt_bytes, char* contents, size_t contents_bytes) {
        shell_line_t line = (shell_line_t) {
                .prompt = prompt,
                .prompt_bytes = prompt_bytes,
                .contents = contents,
                .contents_bytes = contents_bytes,
                .cursor_position = 0 };
        line.contents[line.contents_bytes] = '\0';
        return line;
}

void line_insert(shell_line_t* line, char c) {
        if (0 <= line->cursor_position && line->cursor_position < line->contents_bytes) {
                char current, prev;
                prev = ' ';
                for (int32_t i = line->cursor_position; i < line->contents_bytes; i++) {
                        current = line->contents[i];
                        line->contents[i] = prev;
                        prev = current;
                }
                // Doesn't yet account for c being a control character
                line->contents[line->cursor_position] = c;
                line->cursor_position++;
        }
}

void line_delete(shell_line_t* line) {
        if (0 <= line->cursor_position && line->cursor_position < (line->contents_bytes - 1)) {
                for (int32_t i = line->cursor_position; i < line->contents_bytes; i++) {
                        line->contents[i] = line->contents[i+1];
                }
        }
        line->contents[line->contents_bytes] = '\0';
}

void line_backspace(shell_line_t* line) {
        if (0 < line->cursor_position && line->cursor_position <= line->contents_bytes) {
                for (int32_t i = line->cursor_position - 1; i < line->contents_bytes; i++) {
                        line->contents[i] = line->contents[i+1];
                }
                line->cursor_position--;
        }
        line->contents[line->contents_bytes] = '\0';
}

void line_control_seq(shell_line_t* line) {
        char next = stdio_readchar();
        char after_next = stdio_readchar();
        if (next != '[') {
                line_insert(line, '^');
                line_insert(line, next);
        } else if (after_next == 'D') { // Left arrow
                if (line->cursor_position > 0) {
                        line->cursor_position--;
                }
        } else if (after_next == 'C') { // Right arrow
                if (line->cursor_position < stdstr_length(line->contents)) {
                        line->cursor_position++;
                }
        }
}

void line_print(shell_line_t* line) {
        stdio_print("\33[2K\r");
        stdio_print(line->prompt);
        stdio_print(line->contents);
        int32_t line_length = stdstr_length(line->contents);
        for (int i = line_length; i > line->cursor_position; i--) {
                stdio_printchar('\b');
        }
}

void line_print_with_newline(shell_line_t* line) {
        stdio_print("\33[2K\r");
        stdio_print(line->prompt);
        stdio_print(line->contents);
        stdio_print("\n");
}

void get_line(char* buf, size_t nbytes) {

        shell_line_t line = make_line("mush> ", 6, stdmem_allocate(101), 100);
        line_print(&line);

        char c = stdio_readchar();
        while (c != '\r') {
                // Backspace has some weird legacy mapping: http://www.ibb.net/%7Eanne/keyboard/keyboard.html
                if (c == 'b' || c == 0x7f) {
                        line_backspace(&line);
                } else if (c == 27) {
                        line_control_seq(&line);
                } else {
                        line_insert(&line, c);
                }
                line_print(&line);
                c = stdio_readchar();
        }

        line_print_with_newline(&line);

        stdmem_copy(buf, line.contents, 101);
        buf[nbytes-1] = '\0';
}

void mush() {
        char* last_line = stdmem_allocate(101);
        stdio_print("Welcome to the mu shell\n");
        while (1) {
                get_line(last_line, 101);
                if (stdstr_compare(last_line, "exit") == 0) {
                        _exit(EXIT_SUCCESS);
                } else if (stdstr_compare(last_line, "P0") == 0) {
                        launch_process(entry_P0);
                }
        }
}

void (*entry_sh)() = &mush;
