#include "file.h"

char debug_result[200];

char* debug_list_files() {/*
        int32_t result_index = 0;
        result_index = stdstring_append(debug_result, result_index, "Files: ");

        tailq_inode_t* item;
        char* tmp = stdmem_allocate(sizeof(char)*15);
        TAILQ_FOREACH(item, &open_files, entries) {
                stdstring_int_to_str(item->fd, tmp);
                result_index = stdstring_append(debug_result, result_index, "{");
                result_index = stdstring_append(debug_result, result_index, tmp);
                result_index = stdstring_append(debug_result, result_index, ", ");
                stdstring_int_to_str(item->inode.offset, tmp);
                result_index = stdstring_append(debug_result, result_index, tmp);
                result_index = stdstring_append(debug_result, result_index, ", ");

                result_index = stdstring_append(debug_result, result_index, item->file.pathname);

                result_index = stdstring_append(debug_result, result_index, "} ");
        }

        return debug_result;*/
}
/*
char* debug_priority(int32_t priority) {
        int32_t result_index = 0;
        result_index = stdstring_append(debug_result, result_index, "Priority queue: ");

        if (priority == 0) {
                tailq_pidh_t* item;
                char* tmp = stdmem_allocate(sizeof(char)*15);
                TAILQ_FOREACH(item, &high_priority_queue_head, entries) {
                        stdstring_int_to_str(item->pid, tmp);
                        result_index = stdstring_append(debug_result, result_index, "{");
                        result_index = stdstring_append(debug_result, result_index, tmp);
                        result_index = stdstring_append(debug_result, result_index, "} ");
                }
                return debug_result;

        } else if (priority == 1) {
                tailq_pid_t* item;
                char* tmp = stdmem_allocate(sizeof(char)*15);
                TAILQ_FOREACH(item, &low_priority_queue_head, entries) {
                        stdstring_int_to_str(item->pid, tmp);
                        result_index = stdstring_append(debug_result, result_index, "{");
                        result_index = stdstring_append(debug_result, result_index, tmp);
                        result_index = stdstring_append(debug_result, result_index, "} ");
                }
                return debug_result;

        } else {
                return "";
        }
}*/
