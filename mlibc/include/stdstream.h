#ifndef STDSTREAM_H_
#define STDSTREAM_H_

#include <datastructures/stdqueue.h>
#include <stddef.h>
#include <stdtypes.h>
#include <stdmem.h>

// This is the easiest way to implement a buffer, if not the fastest
struct tailq_stream_buf_s {
        char c;
        TAILQ_ENTRY(tailq_stream_buf_s) entries;
};

typedef struct tailq_stream_buf_s tailq_stream_buf_t;

TAILQ_HEAD(tailq_stream_head, tailq_stream_buf_s);

void stdstream_push_char(struct tailq_stream_head* stream, char c);
char stdstream_pop_char(struct tailq_stream_head* stream);
struct tailq_stream_head* stdstream_initialise_buffer();

#endif
