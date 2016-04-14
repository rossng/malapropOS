#ifndef STDSTR_H_
#define STDSTR_H_

#include <stdtypes.h>

/**
 * @return number of bytes in the null-terminated string (excluding the null byte)
 */
int32_t stdstr_length(char* str);

/**
 * @return -1: first non-matching character in str1 is < that in str2
 *         0: strings are equivalent
 *         1: first non-matching character in str1 is > that in str2
 */
int32_t stdstr_compare(char* str1, char* str2);

void stdstr_int_to_str(int32_t n, char* str);
void stdstr_reverse(char* str);

#endif
