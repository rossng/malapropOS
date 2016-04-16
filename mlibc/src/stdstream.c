#include <stdstream.h>

// This is the easiest way to implement the buffer, if not the fastest
TAILQ_HEAD(tailq_stream_head, tailq_stream_buf_s);

tailq_stream_head* initialise_buffer() {
        struct tailq_stream_head head;
        TAILQ_INIT(&head);
        return head;
}

void buffer_char(filedesc_t f, char c) {
        tailq_stream_buf_t* chr = stdmem_allocate(sizeof(tailq_stream_buf_t));
        chr->c = c;
        if (f == STDIN_FILEDESC) {
                TAILQ_INSERT_TAIL(&stdin_head, chr, entries);
        } if (f == STDOUT_FILEDESC) {
                TAILQ_INSERT_TAIL(&stdout_head, chr, entries);
        } else if (f == STDERR_FILEDESC) {
                TAILQ_INSERT_TAIL(&stderr_head, chr, entries);
        }
}

char retrieve_char(filedesc_t f) {
        char result = '\0';
        tailq_stream_buf_t *chr;
        switch (f) {
                case STDIN_FILEDESC:
                        chr = TAILQ_FIRST(&stdin_head);
                        result = chr->c;
                        TAILQ_REMOVE(&stdin_head, chr, entries);
                        stdmem_free(chr);
                        break;

                case STDOUT_FILEDESC:
                        chr = TAILQ_FIRST(&stdout_head);
                        result = chr->c;
                        TAILQ_REMOVE(&stdout_head, chr, entries);
                        stdmem_free(chr);
                        break;

                case STDERR_FILEDESC:
                        chr = TAILQ_FIRST(&stderr_head);
                        result = chr->c;
                        TAILQ_REMOVE(&stderr_head, chr, entries);
                        stdmem_free(chr);
                        break;

        }
        return result;
}
