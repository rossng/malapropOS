#ifndef __FILE_H
#define __FILE_H

#include <stddef.h>
#include <stdtypes.h>
#include <stdio.h>
#include <stdstream.h>
#include <stdstring.h>
#include <stdmem.h>
#include <stdfile.h>

struct tailq_stream_head* stdin_buffer;
struct tailq_stream_head* stdout_buffer;
struct tailq_stream_head* stderr_buffer;

typedef struct fat16_s {
        uint16_t bytes_per_sector;
        uint8_t sectors_per_cluster;
        uint16_t num_reserved_sectors;
        uint8_t num_file_allocation_tables;
        uint16_t num_root_directory_entries;
        uint16_t total_num_sectors;
        uint8_t media_descriptor;
        uint16_t num_sectors_per_file_allocation_table;
        uint32_t volume_serial_number;
        char volume_label[11];
        char file_system_identifier[9];
} fat16_t;

typedef struct fat16_regions_s {
        // All numbers are measured in sectors
        uint32_t fat_region_start;
        uint32_t root_directory_region_start;
        uint32_t data_region_start;
        uint32_t reserved_region_size;
        uint32_t fat_region_size;
        uint32_t root_directory_region_size;
        uint32_t data_region_size;
} fat16_regions_t;

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
        TAILQ_ENTRY(tailq_open_file_s) entries;
} tailq_open_file_t;

TAILQ_HEAD(tailq_open_files_head_s, tailq_open_file_s);
typedef struct tailq_open_files_head_s tailq_open_files_head_t;
tailq_open_files_head_t* open_files;

int32_t sys_write(int fd, char *ptr, size_t len);
int32_t sys_read(int fd, char *ptr, size_t len);
void file_initialise();

filedesc_t sys_open(char* pathname, int32_t flags);

#endif
