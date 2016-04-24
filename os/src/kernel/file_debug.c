#include "file.h"

char debug_result[400];

char* path = "/MYFILE1.TXT";
char* path2 = "/ANOTHER.DOC";
char* path3 = "/SUBDIR/ASTORY.TXT";
char* path_new = "/SUBDIR/NEWFILE.DOC";
char* example_buffer = "The quick brown fox jumps over the lazy dog.";

char* debug_file_list() {
        int32_t result_index = 0;
        result_index = stdstring_append(debug_result, result_index, "Files: ");

        tailq_open_file_t* item;
        char* tmp = stdmem_allocate(sizeof(char)*15);
        TAILQ_FOREACH(item, open_files, entries) {
                stdstring_int_to_str(item->fd, tmp);
                result_index = stdstring_append(debug_result, result_index, "{ fd: ");
                result_index = stdstring_append(debug_result, result_index, tmp);
                result_index = stdstring_append(debug_result, result_index, ", ofst: ");
                stdstring_int_to_str(item->offset, tmp);
                result_index = stdstring_append(debug_result, result_index, tmp);
                result_index = stdstring_append(debug_result, result_index, ", size: ");
                stdstring_int_to_str(item->directory_entry->file_size_bytes, tmp);
                result_index = stdstring_append(debug_result, result_index, tmp);
                result_index = stdstring_append(debug_result, result_index, ", pth: ");
                result_index = stdstring_append(debug_result, result_index, item->path);
                result_index = stdstring_append(debug_result, result_index, "} ");
        }

        return debug_result;
}

char* read_file(filedesc_t fd, int32_t nbytes) {
        char* buf = stdmem_allocate(nbytes);
        int32_t nbytes_read = sys_read(fd, buf, nbytes);
        return buf;
}

int32_t write_file(filedesc_t fd, char* buf, int32_t nbytes) {
        int32_t nbytes_written = sys_write(fd, buf, nbytes);
        return nbytes_written;
}
