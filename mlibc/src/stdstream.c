#include <stdstream.h>

struct tailq_stream_head* stdstream_initialise_buffer() {
        struct tailq_stream_head* head = stdmem_allocate(sizeof(struct tailq_stream_head));
        TAILQ_INIT(head);
        return head;
}

void stdstream_push_char(struct tailq_stream_head* stream, char c) {
        tailq_stream_buf_t* chr = stdmem_allocate(sizeof(tailq_stream_buf_t));
        chr->c = c;
        TAILQ_INSERT_TAIL(stream, chr, entries);
}

char stdstream_pop_char(struct tailq_stream_head* stream) {
        char result = EOF;
        tailq_stream_buf_t *chr;

        if (!TAILQ_EMPTY(stream)) {
                chr = TAILQ_FIRST(stream);
                result = chr->c;
                TAILQ_REMOVE(stream, chr, entries);
                stdmem_free(chr);
        }
        
        return result;
}
