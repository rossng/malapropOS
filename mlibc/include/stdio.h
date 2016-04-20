#ifndef STDIO_H_
#define STDIO_H_

#include <stdtypes.h>
#include <stddef.h>
#include <stdstr.h>

#define STDIN_FILEDESC 0
#define STDOUT_FILEDESC 1
#define STDERR_FILEDESC 2

void stdio_print(char* str);
void stdio_printchar(char c);
void stdio_printint(int32_t n);

void stdio_file_print(filedesc_t fd, char* str);

size_t stdio_readline(char* buf, size_t nbytes);
char stdio_readchar();
char stdio_readchar_nonblocking();

#endif
