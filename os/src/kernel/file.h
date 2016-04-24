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

tailq_open_files_head_t* open_files;

int32_t sys_write(filedesc_t fd, char *ptr, size_t len);
int32_t sys_read(filedesc_t fd, char *ptr, size_t len);

filedesc_t sys_open(char* pathname, int32_t flags);
int32_t sys_close(filedesc_t fd);
int32_t sys_unlink(char* pathname);
int32_t sys_lseek(filedesc_t fd, int32_t offset, int32_t whence);
tailq_fat16_dir_head_t* sys_getdents(filedesc_t fd, int32_t max_num);

void file_initialise();

#endif
