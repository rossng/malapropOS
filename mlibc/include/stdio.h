#ifndef STDIO_H_
#define STDIO_H_

#include <stdtypes.h>
#include <stdstr.h>

#define STDIN_FILEDESC 0
#define STDOUT_FILEDESC 1
#define STDERR_FILEDESC 2

void stdio_print(char* str);

void stdio_file_print(filedesc_t fd, char* str);

#endif
