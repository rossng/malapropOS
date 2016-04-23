#include "file.h"
#include "../device/PL011.h"
#include "../device/disk.h"

struct tailq_stream_head* stdin_buffer;
struct tailq_stream_head* stdout_buffer;
struct tailq_stream_head* stderr_buffer;

TAILQ_HEAD(tailq_fat16_dir_head_s, tailq_fat16_dir_entry_s);
typedef struct tailq_fat16_dir_head_s tailq_fat16_dir_head_t;

filedesc_t next_fd = 3;

fat16_t* fs;

fat16_t* inspect_file_system() {
        // Don't think there's any easy way to make this truly sector-size-independent
        // TODO: magic numbers!
        uint8_t* boot_sector = stdmem_allocate(512);
        disk_rd(0, boot_sector, 512);
        fat16_t* fs = stdmem_allocate(sizeof(fat16_t));
        fs->bytes_per_sector = boot_sector[0xb] + boot_sector[0xc]*256;
        fs->sectors_per_cluster = boot_sector[0xd];
        fs->num_reserved_sectors = boot_sector[0xe] + boot_sector[0xf]*256;
        fs->num_file_allocation_tables = boot_sector[0x10];
        fs->num_root_directory_entries = boot_sector[0x11] + boot_sector[0x12]*256;
        fs->total_num_sectors = boot_sector[0x13] + boot_sector[0x14]*256;
        fs->media_descriptor = boot_sector[0x15];
        fs->num_sectors_per_file_allocation_table = boot_sector[0x16] + boot_sector[0x17]*256;
        fs->volume_serial_number = boot_sector[0x27] + 256*(boot_sector[0x28] + 256*(boot_sector[0x29] + 256*boot_sector[0x2a]));
        for (int i = 0; i < 11; i++) {
                fs->volume_label[i] = boot_sector[0x2b + i];
        }
        for (int i = 0; i < 8; i++) {
                fs->file_system_identifier[i] = boot_sector[0x36 + i];
        }
        return fs;
}

fat16_regions_t calculate_regions(fat16_t* fs) {
        fat16_regions_t result;
        result.fat_region_start = fs->num_reserved_sectors;
        result.root_directory_region_start = result.fat_region_start + (fs->num_file_allocation_tables * fs->num_sectors_per_file_allocation_table);
        result.data_region_start = result.root_directory_region_start + ((fs->num_root_directory_entries * 32) / fs->bytes_per_sector); // TODO: make sure this rounds up in unusual situtations
        result.reserved_region_size = fs->num_reserved_sectors;
        result.fat_region_size = fs->num_file_allocation_tables * fs->num_sectors_per_file_allocation_table;
        result.root_directory_region_size = (fs->num_root_directory_entries * 32) / fs->bytes_per_sector; // TODO: round up
        result.data_region_size = fs->total_num_sectors - (result.reserved_region_size + result.fat_region_size + result.root_directory_region_size);
        return result;
}

fat16_file_attr_t unpack_file_attributes(uint8_t attributes_byte) {
        fat16_file_attr_t result;
        result.is_read_only = (attributes_byte & 0x01) && 1;
        result.is_hidden_file = (attributes_byte & 0x02) && 1;
        result.is_system_file = (attributes_byte & 0x04) && 1;
        result.is_volume_label = (attributes_byte & 0x08) && 1;
        result.is_subdirectory = (attributes_byte & 0x10) && 1;
        result.is_archive = (attributes_byte & 0x20) && 1;

        return result;
}

uint8_t pack_file_attributes(fat16_file_attr_t attributes) {
        uint8_t result;
        if (attributes.is_read_only) { result |= 0x01; }
        if (attributes.is_hidden_file) { result |= 0x02; }
        if (attributes.is_system_file) { result |= 0x04; }
        if (attributes.is_volume_label) { result |= 0x08; }
        if (attributes.is_subdirectory) { result |= 0x10; }
        if (attributes.is_archive) { result |= 0x20; }

        return result;
}

uint16_t get_fat_entry(fat16_t* fs, uint16_t cluster_number) {
        fat16_regions_t regions = calculate_regions(fs);

        uint32_t in_fat_sector = ((cluster_number * 2) / fs->bytes_per_sector); // entry is in this sector of the FAT
        uint32_t in_sector = regions.fat_region_start + in_fat_sector; // entry is in this sector of the disk
        uint8_t* fat_sector = stdmem_allocate(fs->bytes_per_sector); // buffer to read the sector into
        disk_rd(in_sector, fat_sector, fs->bytes_per_sector);

        uint16_t entries_per_sector = fs->bytes_per_sector / 2;
        uint16_t entry_in_sector = cluster_number - (entries_per_sector * in_fat_sector);

        return fat_sector[entry_in_sector*2] + fat_sector[entry_in_sector*2 + 1]*256;
}

void load_cluster(fat16_t* fs, uint16_t cluster_number, uint8_t* buf) {
        fat16_regions_t regions = calculate_regions(fs);

        // N.B. cluster numbers start from 2 - clusters 0 and 1 do not exist on disk
        uint32_t first_sector = regions.data_region_start + (cluster_number - 2) * fs->sectors_per_cluster;

        for (int i = 0; i < fs->sectors_per_cluster; i++) {
                disk_rd(first_sector + i, &buf[i*fs->bytes_per_sector], fs->bytes_per_sector);
        }
}

fat16_file_data_t load_file(fat16_t* fs, fat16_dir_entry_t* entry) {
        fat16_regions_t regions = calculate_regions(fs);

        uint8_t* file_data = stdmem_allocate(entry->file_size_bytes);
        uint32_t file_clusters_written = 0;

        if (0x0001 < entry->first_cluster && entry->first_cluster < 0xfff0) {

                load_cluster(fs, entry->first_cluster, &file_data[file_clusters_written * fs->sectors_per_cluster * fs->bytes_per_sector]);
                file_clusters_written++;

                uint16_t next_cluster = get_fat_entry(fs, entry->first_cluster);

                while (0x0001 < next_cluster && next_cluster < 0xfff0) {
                        load_cluster(fs, next_cluster, &file_data[file_clusters_written * fs->sectors_per_cluster * fs->bytes_per_sector]);
                        file_clusters_written++;
                        next_cluster = get_fat_entry(fs, next_cluster);
                }
        }

        fat16_file_data_t file;
        file.data = file_data;
        file.num_bytes = file_clusters_written * fs->sectors_per_cluster * fs->bytes_per_sector;

        return file;
}

uint16_t num_clusters_in_chain(fat16_t* fs, uint16_t first_cluster) {
        uint16_t count = 0;

        if (0x0001 < first_cluster && first_cluster < 0xfff0) {
                count++;

                uint16_t next_cluster = get_fat_entry(fs, first_cluster);
                while (0x0001 < next_cluster && next_cluster < 0xfff0) {
                        count++;
                        next_cluster = get_fat_entry(fs, next_cluster);
                }
        }

        return count;
}

// Return true if the directory entries terminate in this cluster
bool add_sector_dir_entries_to_list(fat16_t* fs, uint8_t* sector, tailq_fat16_dir_head_t* entry_list) {
        int16_t entry_bytes = 32;
        int16_t entries_per_sector = fs->bytes_per_sector / entry_bytes;
        int16_t num_sectors = (fs->num_root_directory_entries) / entries_per_sector;

        for (int j = 0; j < entries_per_sector; j++) {
                if (sector[entry_bytes*j] == 0x00) {
                        // Empty - stop searching and mark the search as terminated
                        return 1;
                } else if (sector[entry_bytes*j] == 0xe5) {
                        // File deleted - ignore for now
                        continue;
                } else {
                        // This entry is a file

                        tailq_fat16_dir_entry_t* list_entry = stdmem_allocate(sizeof(tailq_fat16_dir_entry_t));

                        for (int k = 0; k < 8; k++) {
                                list_entry->entry.filename[k] = sector[entry_bytes*j + k] == 0x20 ? '\0' : sector[entry_bytes*j + k];
                        }
                        list_entry->entry.filename[9] = '\0';
                        if (sector[entry_bytes*j] == 0x05) {
                               // The first character of the filename is actually 0xe5 (lol)
                               list_entry->entry.filename[0] = 0xe5;
                        }
                        for (int k = 0; k < 3; k++) {
                                list_entry->entry.extension[k] = sector[entry_bytes*j + 0x08 + k];
                        }
                        list_entry->entry.extension[4] = '\0';
                        list_entry->entry.attributes = sector[entry_bytes*j + 0x0b];
                        list_entry->entry.first_cluster = sector[entry_bytes*j + 0x1a] + sector[entry_bytes*j + 0x1b]*256;
                        list_entry->entry.file_size_bytes = sector[entry_bytes*j + 0x1c] + 256*(sector[entry_bytes*j + 0x1d] + 256*(sector[entry_bytes*j + 0x1e] + 256*sector[entry_bytes*j + 0x1f]));

                        TAILQ_INSERT_TAIL(entry_list, list_entry, entries);
                }
        }

        return 0;
}

tailq_fat16_dir_head_t* get_root_dir(fat16_t* fs) {
        uint8_t* current_sector = stdmem_allocate(fs->bytes_per_sector);

        tailq_fat16_dir_head_t* result = stdmem_allocate(sizeof(tailq_fat16_dir_head_t));
        TAILQ_INIT(result);

        int16_t entry_bytes = 32;
        int16_t entries_per_sector = fs->bytes_per_sector / entry_bytes;
        int16_t num_sectors = (fs->num_root_directory_entries) / entries_per_sector;
        int32_t entries_added = 0;

        bool entries_have_terminated = 0;

        for (int i = 0; i < num_sectors && !entries_have_terminated; i++) {
                disk_rd(1 + (fs->num_sectors_per_file_allocation_table)*(fs->num_file_allocation_tables) + i, current_sector, fs->bytes_per_sector);
                entries_have_terminated = add_sector_dir_entries_to_list(fs, current_sector, result);
        }

        return result;
}

tailq_fat16_dir_head_t* get_dir(fat16_t* fs, fat16_dir_entry_t* entry) {
        fat16_regions_t regions = calculate_regions(fs);

        fat16_file_attr_t attributes = unpack_file_attributes(entry->attributes);

        if (!attributes.is_subdirectory) {
                return NULL;
        }

        tailq_fat16_dir_head_t* result = stdmem_allocate(sizeof(tailq_fat16_dir_head_t));
        TAILQ_INIT(result);

        uint8_t* current_cluster = stdmem_allocate(fs->bytes_per_sector * fs->sectors_per_cluster);
        bool entries_have_terminated = 0;

        if (0x0001 < entry->first_cluster && entry->first_cluster < 0xfff0) {

                load_cluster(fs, entry->first_cluster, current_cluster);

                for (int i = 0; i < fs->sectors_per_cluster && !entries_have_terminated; i++) {
                        entries_have_terminated = add_sector_dir_entries_to_list(fs, &current_cluster[i*fs->bytes_per_sector], result);
                }

                uint16_t next_cluster = get_fat_entry(fs, entry->first_cluster);

                while (0x0001 < next_cluster && next_cluster < 0xfff0 && !entries_have_terminated) {
                        load_cluster(fs, next_cluster, current_cluster);

                        for (int i = 0; i < fs->sectors_per_cluster && !entries_have_terminated; i++) {
                                entries_have_terminated = add_sector_dir_entries_to_list(fs, &current_cluster[i*fs->bytes_per_sector], result);
                        }

                        next_cluster = get_fat_entry(fs, next_cluster);
                }
        }

        return result;
}



void debug_fs() {
        fat16_t* fs = inspect_file_system();
        tailq_fat16_dir_head_t* root_dir = get_root_dir(fs);
        tailq_fat16_dir_entry_t* first_entry = TAILQ_FIRST(root_dir);
        fat16_file_data_t file_data = load_file(fs, &first_entry->entry);

        tailq_fat16_dir_entry_t* directory_entry;
        TAILQ_FOREACH(directory_entry, root_dir, entries) {
                fat16_file_attr_t attributes = unpack_file_attributes(directory_entry->entry.attributes);
                if (attributes.is_subdirectory) {
                        break;
                }
        }

        tailq_fat16_dir_head_t* subdirectory = get_dir(fs, &directory_entry->entry);

        tailq_fat16_dir_entry_t* subdirectory_entry;
        TAILQ_FOREACH(subdirectory_entry, subdirectory, entries) {
                fat16_file_attr_t attributes = unpack_file_attributes(subdirectory_entry->entry.attributes);
        }
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
        fs = inspect_file_system();
        TAILQ_INIT(open_files);
}

tailq_open_file_t* find_open_file(char* pathname) {
        tailq_open_file_t* open_file;
        TAILQ_FOREACH(open_file, open_files, entries) {
                if (stdstring_compare(open_file->path, pathname) == 0) {
                        return open_file;
                }
        }
        return NULL;
}

fat16_file_path_t split_path(char* pathname) {
        if (pathname[0] == '/') {
                pathname = &pathname[1];
        }

        int32_t num_parts = 1;
        for (int i = 0; i < stdstring_length(pathname); i++) {
                if (pathname[i] == '/') {
                        num_parts++;
                }
        }
        char** parts = stdmem_allocate(sizeof(char*)*num_parts);

        int part_num = 0;
        int i = 0;
        while (pathname[i] != '\0') {
                int j = 0;
                while (pathname[i+j] != '/' && pathname[i+j] != '\0') {
                        j++;
                }

                parts[part_num] = stdmem_allocate(sizeof(char)*(j+1));

                stdmem_copy(parts[part_num], &pathname[i], j);
                parts[part_num][j] = '\0';

                i += (j + 1);
                part_num++;
        }

        return (fat16_file_path_t){parts, num_parts};
}

char* descend_path(char* pathname) {
        if (pathname[0] == '/') {
                pathname = &pathname[1];
        }
        int i = 0;
        while (pathname[i] != '/' && pathname[i] != '\0') {
                i++;
        }
        return &pathname[i];
}

/**
 * Remove the filename from the end of a path
 */
char* to_directory(char* pathname) {
        int32_t last_slash_pos = 0;
        int32_t i = 0;
        while (pathname[i] != '\0') {
                if (pathname[i] == '/') {
                        last_slash_pos = i;
                }
                i++;
        }

        char* dir_path = stdmem_allocate(sizeof(char)*(last_slash_pos+1));
        stdmem_copy(dir_path, pathname, last_slash_pos + 1);
        dir_path[last_slash_pos] = '\0';

        return dir_path;
}

/**
 * Just get the filename from the end of a path
 */
char* to_filename(char* pathname) {
        int32_t last_slash_pos = 0;
        int32_t i = 0;
        while (pathname[i] != '\0') {
                if (pathname[i] == '/') {
                        last_slash_pos = i;
                }
                i++;
        }

        char* filename = stdmem_allocate(sizeof(char)*(i - last_slash_pos));
        stdmem_copy(filename, &pathname[last_slash_pos+1], i - last_slash_pos);

        return filename;
}

fat16_file_name_t split_filename(char* filename) {
        fat16_file_name_t result;
        result.name = stdmem_allocate(sizeof(char)*8);
        result.extension = stdmem_allocate(sizeof(char)*3);

        int i = 0;
        while (filename[i] != '.' && i < 8 && filename[i] != '\0') {
                result.name[i] = filename[i];
                i++;
        }
        result.name[i+1] = '\0';

        if (filename[i] == '\0') {
                result.extension[0] = '\0';
        } else {
                i++;
                int j = 0;
                while (filename[i+j] != '\0' && j < 3) {
                        result.extension[j] = filename[i+j];
                        j++;
                }
                result.extension[j+1] = '\0';
        }

        return result;
}

fat16_dir_entry_t* find_file(char* pathname, tailq_fat16_dir_head_t* dir) {
        fat16_dir_entry_t* result = NULL;

        fat16_file_path_t path = split_path(pathname);

        if (path.num_parts > 1) {
                // Search for next directory in the path
                tailq_fat16_dir_entry_t* dir_entry;
                TAILQ_FOREACH(dir_entry, dir, entries) {
                        fat16_file_attr_t attributes = unpack_file_attributes(dir_entry->entry.attributes);
                        if (attributes.is_subdirectory) {
                                if (stdstring_compare(path.parts[0], &(dir_entry->entry.filename[0])) == 0) { // TODO: extensions
                                        tailq_fat16_dir_head_t* next_dir = get_dir(fs, &(dir_entry->entry));
                                        if (next_dir != NULL) {
                                                result = find_file(descend_path(pathname), next_dir);
                                        }
                                }
                        }
                }

        } else if (path.num_parts == 1) {
                // Search for a file in this directory
                tailq_fat16_dir_entry_t* file_entry;
                fat16_file_name_t filename = split_filename(path.parts[0]);
                TAILQ_FOREACH(file_entry, dir, entries) {
                        fat16_file_attr_t attributes = unpack_file_attributes(file_entry->entry.attributes);
                        if (attributes.is_subdirectory) {
                                continue;
                        }
                        if (stdstring_compare(filename.name, &(file_entry->entry.filename[0])) == 0 && stdstring_compare(filename.extension, &(file_entry->entry.extension[0])) == 0) {
                                return &file_entry->entry;
                        }
                }
        }
        return result;
}

fat16_dir_entry_t* find_dir(char* pathname, tailq_fat16_dir_head_t* dir) {
        fat16_dir_entry_t* result = NULL;

        fat16_file_path_t path = split_path(pathname);

        if (path.num_parts > 1) {
                // Search for next directory in the path
                tailq_fat16_dir_entry_t* dir_entry;
                TAILQ_FOREACH(dir_entry, dir, entries) {
                        fat16_file_attr_t attributes = unpack_file_attributes(dir_entry->entry.attributes);
                        if (attributes.is_subdirectory) {
                                if (stdstring_compare(path.parts[0], &(dir_entry->entry.filename[0])) == 0) { // TODO: extensions
                                        tailq_fat16_dir_head_t* next_dir = get_dir(fs, &(dir_entry->entry));
                                        if (next_dir != NULL) {
                                                result = find_dir(descend_path(pathname), next_dir);
                                        }
                                }
                        }
                }

        } else if (path.num_parts == 1) {
                // Search for the directory in this directory
                tailq_fat16_dir_entry_t* file_entry;
                fat16_file_name_t filename = split_filename(path.parts[0]);
                TAILQ_FOREACH(file_entry, dir, entries) {
                        fat16_file_attr_t attributes = unpack_file_attributes(file_entry->entry.attributes);
                        if (attributes.is_subdirectory) {
                                if (stdstring_compare(filename.name, &(file_entry->entry.filename[0])) == 0) { // TODO: extensions
                                        return &file_entry->entry;
                                }
                        }
                }
        }
        return result;
}

filedesc_t get_next_filedesc() {
        return next_fd++;
}

tailq_open_file_t* open_file(char* pathname) {
        tailq_fat16_dir_head_t* root_dir = get_root_dir(fs);
        if (pathname[0] != '/') {
                return NULL;
        } else {
                fat16_dir_entry_t* file = find_file(&pathname[1], root_dir);
                if (file == NULL) {
                        return NULL;
                } else {
                        tailq_open_file_t* opened_file = stdmem_allocate(sizeof(tailq_open_file_t));
                        opened_file->directory_entry = file;
                        opened_file->fd = get_next_filedesc();
                        opened_file->offset = 0;
                        opened_file->path = pathname;
                        TAILQ_INSERT_TAIL(open_files, opened_file, entries);
                        return opened_file;
                }
        }
}

/**
 * -1 = failure, 0 = succes
 */
int32_t create_new_file(char* pathname) {
        tailq_fat16_dir_head_t* root_dir = get_root_dir(fs);
        if (pathname[0] != '/') {
                return -1;
        }

        // If the directory doesn't exist, fail
        fat16_dir_entry_t* dir = find_dir(to_directory(pathname), root_dir);
        if (dir == NULL) {
                return -1;
        }

        to_filename(pathname);

        return -1;

}

char* path = "/MYFILE1.TXT";
char* path2 = "/ANOTHER.DOC";
char* path3 = "/SUBDIR/ASTORY.TXT";

filedesc_t sys_open(char* pathname, int32_t flags) {

        // First, see if the file has already been opened
        tailq_open_file_t* result = find_open_file(pathname);
        if (result == NULL) {
                // If it hasn't, search for the file on disk
                result = open_file(pathname);
                if (result == NULL) {
                        // If the file doesn't exist on disk but the O_CREAT flag
                        // has been passed, create it
                        if (flags & O_CREAT) {
                                if (-1 == create_new_file(pathname)) {
                                        // If the file could not be created, fail
                                        return -1;
                                } else {
                                        // Attempt to open the newly-created file
                                        result = open_file(pathname);
                                        if (result == NULL) {
                                                // If it does not exist, fail
                                                return -1;
                                        }
                                }
                        } else {
                                // If the O_CREATE flag was not passed, fail
                                return -1;
                        }
                }
        }

        if (flags & O_APPEND) { // Move offset to end of file
                result->offset = result->directory_entry->file_size_bytes;
        } else { // Move offset to beginning of file
                result->offset = 0;
        }

        return result->fd;
}
