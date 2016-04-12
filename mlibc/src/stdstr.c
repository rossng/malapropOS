#include <stdstr.h>

int32_t stdstr_length(char* str) {
        int32_t len = 0;
        while (str[len] != '\0') {
                len++;
        }
        return len;
}
