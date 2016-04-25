/* MUSH: the μ shell */

#include "sh.h"
#include "P0.h"
#include "P1.h"
#include "ls.h"
#include "cd.h"
#include "pwd.h"
#include "cat.h"
#include "mkdir.h"
#include "touch.h"
#include "write.h"
#include "append.h"
#include "rm.h"
#include "messenger.h"
#include "producer.h"
#include "pipeline.h"
#include "consumer.h"
#include "syscall.h"
#include "writemany.h"
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
void launch_process(proc_ptr function, int32_t argc, char* argv[], int32_t priority) {
        pid_t* stored_pid = stdmem_allocate(sizeof(pid_t));
        uint32_t fp;
        asm volatile(
                "mov %0, r11 \n"
                : "=r" (fp)
                :
                : "r0"
        );
        int32_t fork_pid = _fork(fp);
        if (fork_pid == 0) {
                // If this is the child process, exec the new process
                _exec(function, argc, argv);
        } else {
                *stored_pid = fork_pid;

                // Otherwise, wait for the child to complete
                _setpriority(*stored_pid, 1, priority); // TODO: should really get the current pid dynamically (1 is a magic number)

                pid_t result = 0;
                bool waiting = 1;
                do {
                        result = _waitpid(PROCESS_EVENT_EXITED, *stored_pid, WAITPID_NOHANG);
                        if (!result) {
                                handle_input(*stored_pid);
                                _yield();
                        } else {
                                waiting = 0;
                        }
                } while (waiting);
        }
}

void launch_process_bg(proc_ptr function, int32_t argc, char* argv[]) {
        uint32_t fp;
        asm volatile(
                "mov %0, r11 \n"
                : "=r" (fp)
                :
                : "r0"
        );
        int32_t fork_pid = _fork(fp);
        if (fork_pid == 0) {
                _exec(function, argc, argv);
        } else {
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

shell_command_t* parse_shell_command(char* line) {
        shell_command_t* result = stdmem_allocate(sizeof(shell_command_t));
        result->argc = 0;
        result->argv = stdmem_allocate(sizeof(char*)*10); // space for 10 arguments (TODO: allow infinite)
        result->background = 0;
        result->priority = 1;

        token_t* token = stdstring_next_token(line, " ");

        char* name = stdmem_allocate(sizeof(char)*(stdstring_length(token->token_start) + 1));
        stdmem_copy(name, token->token_start, stdstring_length(token->token_start) + 1);

        result->name = name;

        while (token->after_token != NULL && result->argc < 10) {
                token = stdstring_next_token(token->after_token, " ");
                int32_t token_length = stdstring_length(token->token_start);
                if (stdstring_compare(token->token_start, "&") == 0) {
                        result->background = 1;
                        break;
                }
                if (stdstring_compare(token->token_start, "!") == 0) {
                        result->priority = 0;
                        break;
                }
                if (token_length > 0) {
                        result->argv[result->argc] = stdmem_allocate(sizeof(char)*(token_length + 1));
                        stdmem_copy(result->argv[result->argc], token->token_start, token_length + 1);
                        result->argc++;
                }
        }

        return result;
}

proc_ptr choose_process(char* name) {
        if (stdstring_compare("P0", name) == 0) {
                return entry_P0;
        }
        if (stdstring_compare("P1", name) == 0) {
                return entry_P1;
        }
        if (stdstring_compare("ls", name) == 0) {
                return entry_ls;
        }
        if (stdstring_compare("cd", name) == 0) {
                return entry_cd;
        }
        if (stdstring_compare("pwd", name) == 0) {
                return entry_pwd;
        }
        if (stdstring_compare("cat", name) == 0) {
                return entry_cat;
        }
        if (stdstring_compare("mkdir", name) == 0) {
                return entry_mkdir;
        }
        if (stdstring_compare("touch", name) == 0) {
                return entry_touch;
        }
        if (stdstring_compare("write", name) == 0) {
                return entry_write;
        }
        if (stdstring_compare("rm", name) == 0) {
                return entry_rm;
        }
        if (stdstring_compare("append", name) == 0) {
                return entry_append;
        }
        if (stdstring_compare("messenger", name) == 0) {
                return entry_messenger;
        }
        if (stdstring_compare("producer", name) == 0) {
                return entry_producer;
        }
        if (stdstring_compare("pipeline", name) == 0) {
                return entry_pipeline;
        }
        if (stdstring_compare("consumer", name) == 0) {
                return entry_consumer;
        }
        if (stdstring_compare("writemany", name) == 0) {
                return entry_writemany;
        }
        return NULL;
}

void mush() {
        sh_stdin_buffer = stdstream_initialise_buffer();
        char* last_line = stdmem_allocate(101);
        stdio_print("Welcome to the mu shell\n");
        while (1) {
                get_line(last_line, 101);
                // TODO: make this sane
                shell_command_t* command = parse_shell_command(last_line);
                proc_ptr function = choose_process(command->name);

                if (function == NULL) {
                        stdio_print("Command not recognised.");
                } else if (command->background) {
                        launch_process_bg(function, command->argc, command->argv);
                } else {
                        launch_process(function, command->argc, command->argv, command->priority);
                }
        }
}

void (*entry_sh)() = &mush;
