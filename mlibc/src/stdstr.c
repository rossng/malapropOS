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
