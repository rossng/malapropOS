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

//TAILQ_HEAD(tailq_inode_head_t, tailq_inode_s);
//struct tailq_inode_head_t open_files;

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

typedef struct fat16_dir_entry_s {
        char filename[8];
        char extension[3];
        uint8_t attributes;
        // Ignore updated and created dates
        uint16_t first_cluster;
        uint32_t file_size_bytes;
} fat16_dir_entry_t;

typedef struct fat16_dir_s {
        fat16_dir_entry_t* entries;
        int16_t num_entries;
} fat16_dir_t;

int32_t sys_write(int fd, char *ptr, size_t len);
int32_t sys_read(int fd, char *ptr, size_t len);
void file_initialise();

filedesc_t sys_open(char* pathname, int32_t flags);

#endif
