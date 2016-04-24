#ifndef __STDFILE_H
#define __STDFILE_H

#define O_CREAT 0x1
#define O_APPEND 0x2

#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR 3

#define SEEK_SET 1
#define SEEK_CUR 2
#define SEEK_END 3

#include <stdtypes.h>
#include <datastructures/stdqueue.h>

typedef struct fat16_dir_entry_s {
        char filename[9];
        char extension[4];
        uint8_t attributes;
        // Ignore updated and created dates
        uint16_t first_cluster;
        uint32_t file_size_bytes;
} fat16_dir_entry_t;

typedef struct tailq_fat16_dir_entry_s {
        fat16_dir_entry_t entry;
        TAILQ_ENTRY(tailq_fat16_dir_entry_s) entries;
} tailq_fat16_dir_entry_t;

typedef struct fat16_file_attr_s {
        bool is_read_only;
        bool is_hidden_file;
        bool is_system_file;
        bool is_volume_label;
        bool is_subdirectory;
        bool is_archive;
} fat16_file_attr_t;

typedef struct fat16_file_data_s {
        uint8_t* data;
        uint32_t num_bytes;
} fat16_file_data_t;

typedef struct fat16_file_clusters_s {
        uint16_t* cluster_address; // Array of cluster addresses
        uint32_t num_clusters;
} fat16_file_clusters_t;

typedef struct fat16_file_path_s {
        char** parts;
        uint32_t num_parts;
} fat16_file_path_t;

typedef struct fat16_file_name_s {
        char* name;
        char* extension;
} fat16_file_name_t;

typedef struct tailq_open_file_s {
        filedesc_t fd;
        fat16_dir_entry_t* directory_entry;
        char* path;
        int32_t offset;
        bool append;
        TAILQ_ENTRY(tailq_open_file_s) entries;
} tailq_open_file_t;

TAILQ_HEAD(tailq_open_files_head_s, tailq_open_file_s);
typedef struct tailq_open_files_head_s tailq_open_files_head_t;

TAILQ_HEAD(tailq_fat16_dir_head_s, tailq_fat16_dir_entry_s);
typedef struct tailq_fat16_dir_head_s tailq_fat16_dir_head_t;

#endif
