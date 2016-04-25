#ifndef __IPC_H
#define __IPC_H

#include <stdtypes.h>
#include <stddef.h>
#include <datastructures/stdqueue.h>

/**
 * Asynchronously send a message to another process.
 * @return 0 if the message was sent successfully, -1 if there was an error
 */
int32_t ipc_send_message(char* buf, size_t nbytes, pid_t from, pid_t to);

/**
 * Asynchronously receive a message from another process.
 * @return 0 if there was a message to receive, -1 if there was no message to receive
 */
int32_t ipc_receive_message_from(char* buf, size_t nbytes, pid_t from, pid_t to);
int32_t ipc_receive_any_message(char* buf, size_t nbytes, pid_t to);

void ipc_initialise();

typedef struct tailq_message_s {
        char* content;
        size_t nbytes;
        pid_t from;
        pid_t to;
        TAILQ_ENTRY(tailq_message_s) entries;
} tailq_message_t;


TAILQ_HEAD(tailq_messages_head_s, tailq_message_s);
typedef struct tailq_messages_head_s tailq_messages_head_t;

tailq_messages_head_t* messages;

#endif
