#include "file.h"
#include "../device/PL011.h"
#include "../device/disk.h"

struct tailq_stream_head* stdin_buffer;
struct tailq_stream_head* stdout_buffer;
struct tailq_stream_head* stderr_buffer;

TAILQ_HEAD(tailq_fat16_dir_head_s, tailq_fat16_dir_entry_s);
typedef struct tailq_fat16_dir_head_s tailq_fat16_dir_head_t;

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
        result.is_read_only = attributes_byte & 0x01;
        result.is_hidden_file = attributes_byte & 0x02;
        result.is_system_file = attributes_byte & 0x04;
        result.is_volume_label = attributes_byte & 0x08;
        result.is_subdirectory = attributes_byte & 0x10;
        result.is_archive = attributes_byte & 0x20;

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
                                list_entry->entry.filename[k] = sector[entry_bytes*j + k];
                        }
                        if (sector[entry_bytes*j] == 0x05) {
                               // The first character of the filename is actually 0xe5 (lol)
                               list_entry->entry.filename[0] = 0xe5;
                        }
                        for (int k = 0; k < 3; k++) {
                                list_entry->entry.extension[k] = sector[entry_bytes*j + 0x08 + k];
                        }
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
