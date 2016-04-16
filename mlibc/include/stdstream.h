#ifndef STDSTREAM_H_
#define STDSTREAM_H_

#include <stddef.h>
#include <stdio.h>
#include <stdtypes.h>
#include <datastructures/stdqueue.h>
#include <stdmem.h>

void buffer_char(filedesc_t f, char c);
char retrieve_char(filedesc_t f);
void initialise_buffers();

typedef struct tailq_stream_buf_s {
        char c;
        TAILQ_ENTRY(tailq_stream_buf_s) entries;
} tailq_stream_buf_t;

#endif
