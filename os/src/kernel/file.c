#include "file.h"
#include "../device/PL011.h"
#include "../device/disk.h"

struct tailq_stream_head* stdin_buffer;
struct tailq_stream_head* stdout_buffer;
struct tailq_stream_head* stderr_buffer;

//struct tailq_inode_head_t open_files;

fat16_t* inspect_file_system() {
        // Don't think there's any easy way to make this truly sector-size-independent
        // TODO: magic numbers!
        uint8_t* boot_sector = stdmem_allocate(512);
        disk_rd(0, boot_sector, 512);
        fat16_t* fs = stdmem_allocate(sizeof(fat16_t));
        fs->bytes_per_sector = boot_sector[0xb]*256 + boot_sector[0xc];
        fs->sectors_per_cluster = boot_sector[0xd];
        fs->num_reserved_sectors = boot_sector[0xe]*256 + boot_sector[0xf];
        fs->num_file_allocation_tables = boot_sector[0x10];
        fs->num_root_directory_entries = boot_sector[0x11]*256 + boot_sector[0x12];
        fs->total_num_sectors = boot_sector[0x13]*256 + boot_sector[0x14];
        fs->media_descriptor = boot_sector[0x15];
        fs->num_sectors_per_file_allocation_table = boot_sector[0x16]*256 + boot_sector[0x17];
        fs->volume_serial_number = ((boot_sector[0x27]*256 + boot_sector[0x28])*256 + boot_sector[0x29])*256 + boot_sector[0x2a];
        for (int i = 0; i < 11; i++) {
                fs->volume_label[i] = boot_sector[0x2b + i];
        }
        for (int i = 0; i < 8; i++) {
                fs->file_system_identifier[i] = boot_sector[0x36 + i];
        }
        return fs;
}

fat16_dir_t* get_root_dir(fat16_t* fs) {
        uint8_t* current_sector = stdmem_allocate(fs->bytes_per_sector);
        fat16_dir_t* result = stdmem_allocate(sizeof(fat16_dir_t));
        result->entries = stdmem_allocate(sizeof(fat16_dir_entry_t)*fs->num_root_directory_entries);

        int16_t entry_bytes = 32;
        int16_t entries_per_sector = fs->bytes_per_sector / entry_bytes;
        int16_t num_sectors = (fs->num_root_directory_entries) / entries_per_sector;
        int32_t entries_added = 0;
        for (int i = 0; i < num_sectors; i++) {
                disk_rd(1 + (fs->num_sectors_per_file_allocation_table)*(fs->num_file_allocation_tables) + i, current_sector, fs->bytes_per_sector);

                for (int j = 0; j < entries_per_sector; j++) {
                        if (current_sector[entry_bytes*j] == 0x00) {
                                continue;
                        } else if (current_sector[entry_bytes*j] == 0xe5) {
                                // File deleted - ignore for now
                                continue;
                        } else {
                                // This entry is a file
                                for (int k = 0; k < 8; k++) {
                                        result->entries[entries_added].filename[k] = current_sector[entry_bytes*j + k];
                                }
                                if (current_sector[entry_bytes*j] == 0x05) {
                                       // The first character of the filename is actually 0xe5 (lol)
                                       result->entries[entries_added].filename[0] = 0xe5;
                               }
                                for (int k = 0; k < 3; k++) {
                                        result->entries[entries_added].extension[k] = current_sector[entry_bytes*j + 0x08 + k];
                                }
                                result->entries[entries_added].attributes = current_sector[entry_bytes*j + 0x0b];
                                result->entries[entries_added].first_cluster = current_sector[entry_bytes*j + 0x1a]*256 + current_sector[entry_bytes*j + 0x1b];
                                result->entries[entries_added].file_size_bytes = ((current_sector[entry_bytes*j + 0x1c]*256 + current_sector[entry_bytes*j + 0x1d])*256 + current_sector[entry_bytes*j + 0x1e])*256 + current_sector[entry_bytes*j + 0x1f];
                                entries_added++;
                        }
                }
        }
        result->num_entries = entries_added;

        return result;
}

void debug_fs() {
        fat16_t* fs = inspect_file_system();
        fat16_dir_t* root_dir = get_root_dir(fs);
}


int32_t sys_write(int fd, char *buf, size_t nbytes) {
        // If printing to stdout
        if (STDOUT_FILEDESC == fd) {
                int32_t i;
                for (i = 0; i < nbytes; i++) {
                        PL011_putc(UART0, *buf++);
                }
                return i;
        }
        // Otherwise indicate failure
        return -1;
}

int32_t sys_read(int fd, char *buf, size_t nbytes) {
        // If reading from stdin
        if (STDIN_FILEDESC == fd) {
                int32_t i = 0;
                while (i < nbytes) {
                        buf[i] = stdstream_pop_char(stdin_buffer);
                        i++;
                }
                return i;
        }/* else if (is_file_open(fd)) {
                disk_rd(find_open_file(fd)->start_block, buf, nbytes);
        }*/ else {
                return -1;
        }
}

void file_initialise() {
        stdin_buffer = stdstream_initialise_buffer();
        stdout_buffer = stdstream_initialise_buffer();
        stderr_buffer = stdstream_initialise_buffer();
        //TAILQ_INIT(&open_files);
}

filedesc_t sys_open(char* pathname, int32_t flags) {
        /*
        inode_t* result = find_open_file(pathname);
        if (result == NULL) {
                if (flags & O_CREAT) {
                        result = create_new_file(pathname);
                } else {
                        return -1;
                }
        }

        if (flags & O_APPEND) { // Move offset to end of file
                result->offset = 0; // TODO
        } else { // Move offset to beginning of file
                result->offset = 0;
        }

        return result->fd;*/
}
