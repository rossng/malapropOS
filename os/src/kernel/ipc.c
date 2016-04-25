#include "ipc.h"

tailq_messages_head_t* messages;

int32_t ipc_send_message(char* buf, size_t nbytes, pid_t from, pid_t to) {
        tailq_message_t* message = stdmem_allocate(sizeof(tailq_message_t));
        message->content = stdmem_allocate(nbytes);
        stdmem_copy(message->content, buf, nbytes);
        message->nbytes = nbytes;
        message->from = from;
        message->to = to;
        TAILQ_INSERT_TAIL(messages, message, entries);
}

int32_t ipc_receive_message_from(char* buf, size_t nbytes, pid_t from, pid_t to) {
        tailq_message_t* message;
        TAILQ_FOREACH(message, messages, entries) {
                if (message->from == from && message->to == to) {
                        stdmem_copy(buf, message->content, nbytes);
                        TAILQ_REMOVE(messages, message, entries);
                        return 0;
                }
        }
        return -1;
}

int32_t ipc_receive_any_message(char* buf, size_t nbytes, pid_t to) {
        tailq_message_t* message;
        TAILQ_FOREACH(message, messages, entries) {
                if (message->to == to) {
                        stdmem_copy(buf, message->content, nbytes);
                        TAILQ_REMOVE(messages, message, entries);
                        return 0;
                }
        }
        return -1;
}

void ipc_initialise() {
        messages = stdmem_allocate(sizeof(tailq_messages_head_t));
        TAILQ_INIT(messages);
}

char ipc_debug_result[200];

char* debug_message_list() {
        int32_t result_index = 0;
        result_index = stdstring_append(ipc_debug_result, result_index, "Messages: ");

        tailq_message_t* message;
        char* tmp = stdmem_allocate(sizeof(char)*15);
        TAILQ_FOREACH(message, messages, entries) {
                stdstring_int_to_str(message->from, tmp);
                result_index = stdstring_append(ipc_debug_result, result_index, "{");
                result_index = stdstring_append(ipc_debug_result, result_index, tmp);
                result_index = stdstring_append(ipc_debug_result, result_index, "->");
                stdstring_int_to_str(message->to, tmp);
                result_index = stdstring_append(ipc_debug_result, result_index, tmp);
                result_index = stdstring_append(ipc_debug_result, result_index, ", ");
                result_index = stdstring_append(ipc_debug_result, result_index, message->content);

                result_index = stdstring_append(ipc_debug_result, result_index, "} ");
        }

        return ipc_debug_result;
}
