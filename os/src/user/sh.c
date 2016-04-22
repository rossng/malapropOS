/* MUSH: the μ shell */

#include "sh.h"
#include "P0.h"
#include "P1.h"
#include "P2.h"
#include "syscall.h"
#include "../device/PL011.h"
#include <stdstream.h>

struct tailq_stream_head* sh_stdin_buffer;

void handle_input(pid_t child_pid) {
        char c = EOF;
        do {
                c = stdio_readchar_nonblocking();
                if (c == 3) { // Ctrl+C
                        // Normally we send SIGINT on Ctrl+C, but at the moment
                        // we don't care about letting the child process handle
                        // the signal - just kill it with the scheduler
                        _kill(child_pid, SIGKILL);
                }
                if (c != EOF) {
                        stdstream_push_char(sh_stdin_buffer, c);
                }
        } while (c != EOF);
}

char get_next_char() {
        char c = stdstream_pop_char(sh_stdin_buffer);
        if (c == EOF) {
                c = stdio_readchar();
        }
        return c;
}

// See http://brennan.io/2015/01/16/write-a-shell-in-c/ for more info on basic shell implementation
void launch_process(void (*function)(), int32_t priority) {
        pid_t fork_pid = _fork();
        int32_t status;
        if (fork_pid == 0) {
                // If this is the child process, exec the new process
                _exec(function);
        } else {
                // Otherwise, wait for the child to complete
                pid_t child_pid = fork_pid;
                _setpriority(child_pid, 1, priority); // TODO: should really get the current pid dynamically (1 is a magic number)
                pid_t result = 0;
                bool waiting = 1;
                do {
                        result = _waitpid(PROCESS_EVENT_EXITED, child_pid, WAITPID_NOHANG);
                        if (!result) {
                                handle_input(child_pid);
                                _yield();
                        } else {
                                waiting = 0;
                        }
                } while (waiting);
        }
}

void (*bg_process)();
void launch_process_bg(void (*function)()) {
        // Both copying the stack and COW are too complicated, just store the argument in a global
        // so it's preserved on both sides of the fork. Obviously going to be some nasty race conditions here.
        bg_process = function;
        pid_t fork_pid = _fork();
        if (fork_pid == 0) {
                _exec(bg_process);
        } else {
                // The parent may get rescheduled first, in which case we must
                // immediately yield() to let the the child process exec() and
                // get a new stack - otherwise the child process will overwrite
                // values in the parent stack when it returns from _fork().
                // This is a bit of a hack, but necessitated by the fact we don't
                // have copy-on-write.
                // If we are at the top of the process queue, a call to yield may
                // schedule this process again, so first get us to the back of the
                // queue
                _yield();
                // Then let the other process take over
                _yield();
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
        char next = get_next_char();
        char after_next = get_next_char();
        if (next != '[') {
                line_insert(line, '^');
                line_insert(line, next);
        } else if (after_next == 'D') { // Left arrow
                if (line->cursor_position > 0) {
                        line->cursor_position--;
                }
        } else if (after_next == 'C') { // Right arrow
                if (line->cursor_position < stdstring_length(line->contents)) {
                        line->cursor_position++;
                }
        }
}

void line_print(shell_line_t* line) {
        stdio_print("\33[2K\r");
        stdio_print(line->prompt);
        stdio_print(line->contents);
        int32_t line_length = stdstring_length(line->contents);
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

        char c = get_next_char();
        while (c != '\r') {
                if (c == 3) {                           // Ctrl+C
                        buf[0] = '\0';
                        return;
                } else if (c == 'b' || c == 0x7f) {     // Backspace
                        // Backspace has some weird legacy mapping: http://www.ibb.net/%7Eanne/keyboard/keyboard.html
                        line_backspace(&line);
                } else if (c == 27) {                   // Start of a control sequence
                        line_control_seq(&line);
                } else {                                // Any other character
                        line_insert(&line, c);
                }
                line_print(&line);
                c = get_next_char();
        }

        line_print_with_newline(&line);

        stdmem_copy(buf, line.contents, 101);
        buf[nbytes-1] = '\0';
}

void mush() {
        sh_stdin_buffer = stdstream_initialise_buffer();
        char* last_line = stdmem_allocate(101);
        stdio_print("Welcome to the mu shell\n");
        while (1) {
                get_line(last_line, 101);
                // TODO: make this sane
                token_t* token = stdstring_next_token(last_line, " ");
                if (stdstring_compare(token->token_start, "exit") == 0) {
                        _exit(EXIT_SUCCESS);
                } else if (stdstring_compare(token->token_start, "P0") == 0) {
                        if (token->after_token != NULL) {
                                token = stdstring_next_token(token->after_token, " ");
                                if (stdstring_compare(token->token_start, "&") == 0) { // Background, low priority
                                        launch_process_bg(entry_P0);
                                } else if (stdstring_compare(token->token_start, "!") == 0) { // Foreground, high priority
                                        launch_process(entry_P0, 0);
                                } else {
                                        stdio_print("Invalid launch options\n");
                                }
                        } else { // Foreground, low priority
                                launch_process(entry_P0, 1);
                        }
                } else if (stdstring_compare(token->token_start, "P1") == 0) {
                        if (token->after_token != NULL) {
                                token = stdstring_next_token(token->after_token, " ");
                                if (stdstring_compare(token->token_start, "&") == 0) {
                                        launch_process_bg(entry_P1);
                                } else if (stdstring_compare(token->token_start, "!") == 0) {
                                        launch_process(entry_P1, 0);
                                } else {
                                        stdio_print("Invalid launch options\n");
                                }
                        } else {
                                launch_process(entry_P1, 1);
                        }
                } else if (stdstring_compare(token->token_start, "P2") == 0) {
                        if (token->after_token != NULL) {
                                token = stdstring_next_token(token->after_token, " ");
                                if (stdstring_compare(token->token_start, "&") == 0) {
                                        launch_process_bg(entry_P2);
                                } else if (stdstring_compare(token->token_start, "!") == 0) {
                                        launch_process(entry_P2, 0);
                                } else {
                                        stdio_print("Invalid launch options\n");
                                }
                        } else {
                                launch_process(entry_P2, 1);
                        }
                } else if (stdstring_length(token->token_start) == 0) {
                        continue;
                } else {
                        stdio_print("Command not recognised\n");
                };
        }
}

void (*entry_sh)() = &mush;
