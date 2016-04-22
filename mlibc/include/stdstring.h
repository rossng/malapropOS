#ifndef stdstring_H_
#define stdstring_H_

#include <stdtypes.h>
#include <stdmem.h>
#include <stddef.h>

/**
 * @return number of bytes in the null-terminated string (excluding the null byte)
 */
int32_t stdstring_length(char* str);

/**
 * @return -1: first non-matching character in str1 is < that in str2
 *         0: strings are equivalent
 *         1: first non-matching character in str1 is > that in str2
 */
int32_t stdstring_compare(char* str1, char* str2);

void stdstring_int_to_str(int32_t n, char* str);
void stdstring_reverse(char* str);



typedef struct {
        char* token_start;
        char* after_token;
} token_t;

bool stdstr_contains_char(char c, const char* str);

token_t* stdstring_next_token(char* str, const char* delimeters);

int32_t stdstring_append(char* dest, int32_t dest_index, char* source);

#endif
