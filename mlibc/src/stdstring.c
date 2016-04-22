#include <stdstring.h>

int32_t stdstring_length(char* str) {
        int32_t len = 0;
        while (str[len] != '\0') {
                len++;
        }
        return len;
}


int32_t stdstring_compare(char* str1, char* str2) {
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
void stdstring_int_to_str(int32_t n, char* str) {
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
        stdstring_reverse(str);
}

void stdstring_reverse(char* str) {
        int i, j;
        char c;

        for (i = 0, j = stdstring_length(str)-1; i < j; i++, j--) {
                c = str[i];
                str[i] = str[j];
                str[j] = c;
        }
}

bool stdstr_contains_char(char c, const char* str) {
        int32_t i = 0;
        while (str[i] != '\0') {
                if (c == str[i]) {
                        return 1;
                }
                i++;
        }
        return 0;
}

/**
 * Heavily inspired by strtok
 */
token_t* stdstring_next_token(char* str, const char* delimeters) {
        int32_t i = 0;
        token_t* token = stdmem_allocate(sizeof(token_t));
        // Find the start of the next token
        while (str[i] != '\0' && stdstr_contains_char(str[i], delimeters)) {
                i++;
        }
        token->token_start = &(str[i]);
        // Find the end of the token and add the string terminator there
        while (str[i] != '\0' && !stdstr_contains_char(str[i], delimeters)) {
                i++;
        }
        if (str[i] != '\0') {
                str[i] = '\0';
                token->after_token = &(str[i+1]);
        } else {
                token->after_token = NULL;
        }

        return token;
}

/**
 * @return position of new terminating null
 */
int32_t stdstring_append(char* dest, int32_t dest_index, char* source) {
        int32_t source_index = 0;
        while (source[source_index] != '\0') {
                dest[dest_index + source_index] = source[source_index];
                source_index++;
        }
        dest[dest_index + source_index] = '\0';

        return dest_index + source_index;
}
