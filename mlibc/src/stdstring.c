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


/*
 * Convert a string to a long integer. From the Apple XNU source: http://opensource.apple.com//source/xnu/xnu-1456.1.26/bsd/libkern/strtol.c
 */
#define LONG_MIN ((long) 0x80000000L)
#define LONG_MAX 0x7FFFFFFFL

int isupper (char c) {
     return (c >= 'A' && c <= 'Z');
}

int isalpha (char c) {
     return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}


int isspace (char c) {
     return (c == ' ' || c == '\t' || c == '\n' || c == '\12');
}

int isdigit (char c) {
     return (c >= '0' && c <= '9');
}

long stdstring_strtol(const char *nptr, char **endptr, int base) {
	const char *s = nptr;
	unsigned long acc;
	int c;
	unsigned long cutoff;
	int neg = 0, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	} else if ((base == 0 || base == 2) &&
	    c == '0' && (*s == 'b' || *s == 'B')) {
		c = s[1];
		s += 2;
		base = 2;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long)base;
	cutoff /= (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || acc == cutoff && c > cutlim)
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LONG_MIN : LONG_MAX;
//		errno = ERANGE;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *)(any ? s - 1 : nptr);
	return (acc);
}
