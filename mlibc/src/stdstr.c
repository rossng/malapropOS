#include <stdstr.h>

int32_t stdstr_length(char* str) {
        int32_t len = 0;
        while (str[len] != '\0') {
                len++;
        }
        return len;
}


int32_t stdstr_compare(char* str1, char* str2) {
        int i = 0;
        while (1) {
                if (str1[i] == '\0' && str2[i] == '\0') {
                        return 0;
                } else if (str1[i] == '\0' && str2[i] != '\0') {
                        return -1;
                } else if (str1[i] != '\0' && str2[i] == '\0') {
                        return 1;
                } else if (str1[i] < str2[i]) {
                        return -1;
                } else if (str1[i] > str2[i]) {
                        return 1;
                }
                i++;
        }
}

// Implementation based on K&R: see https://en.wikibooks.org/wiki/C_Programming/C_Reference/stdlib.h/itoa
// A sufficiently long buffer must be allocated at str by the caller.
void stdstr_int_to_str(int32_t n, char* str) {
        int i = 0;
        int sign = n;

        if (n < 0) {
                n = -n;
        }

        do {
                str[i++] = n % 10 + '0';
        } while ((n /= 10) > 0);

        if (sign < 0) {
                str[i++] = '-';
        }

        str[i] = '\0';
        stdstr_reverse(str);
}

void stdstr_reverse(char* str) {
        int i, j;
        char c;

        for (i = 0, j = stdstr_length(str)-1; i < j; i++, j--) {
                c = str[i];
                str[i] = str[j];
                str[j] = c;
        }
}
