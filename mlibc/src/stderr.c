#include <stderr.h>

void panic(char* message, ...) {
        // TODO: implement with printf
        stdio_print(message);
        while (1) {}
}
