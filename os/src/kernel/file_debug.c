#include "file.h"

char debug_result[200];

char* debug_file_list() {
        int32_t result_index = 0;
        result_index = stdstring_append(debug_result, result_index, "Files: ");

        tailq_open_file_t* item;
        char* tmp = stdmem_allocate(sizeof(char)*15);
        TAILQ_FOREACH(item, open_files, entries) {
                stdstring_int_to_str(item->fd, tmp);
                result_index = stdstring_append(debug_result, result_index, "{");
                result_index = stdstring_append(debug_result, result_index, tmp);
                result_index = stdstring_append(debug_result, result_index, ", ");
                stdstring_int_to_str(item->offset, tmp);
                result_index = stdstring_append(debug_result, result_index, tmp);
                result_index = stdstring_append(debug_result, result_index, ", ");

                result_index = stdstring_append(debug_result, result_index, item->path);

                result_index = stdstring_append(debug_result, result_index, "} ");
        }

        return debug_result;
}
