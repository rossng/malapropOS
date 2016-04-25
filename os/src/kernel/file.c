#include "file.h"
#include "../device/PL011.h"
#include "../device/disk.h"

struct tailq_stream_head* stdin_buffer;
struct tailq_stream_head* stdout_buffer;
struct tailq_stream_head* stderr_buffer;

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

uint16_t get_successor_cluster(fat16_t* fs, uint16_t cluster_number) {
        fat16_regions_t regions = calculate_regions(fs);

        uint32_t in_fat_sector = ((cluster_number * 2) / fs->bytes_per_sector); // entry is in this sector of the FAT
        uint32_t in_sector = regions.fat_region_start + in_fat_sector; // entry is in this sector of the disk
        uint8_t* fat_sector = stdmem_allocate(fs->bytes_per_sector); // buffer to read the sector into
        disk_rd(in_sector, fat_sector, fs->bytes_per_sector);

        uint16_t entries_per_sector = fs->bytes_per_sector / 2;
        uint16_t entry_in_sector = cluster_number - (entries_per_sector * in_fat_sector);

        return fat_sector[entry_in_sector*2] + fat_sector[entry_in_sector*2 + 1]*256;
}

/**
 * Sets the FAT entry for the specified cluster to the provided value.
 * Returns -1 on failure, 0 on success
 */
int32_t set_successor_cluster(fat16_t* fs, uint16_t cluster_number, uint16_t successor_cluster_number) {
        fat16_regions_t regions = calculate_regions(fs);

        uint32_t in_fat_sector = ((cluster_number * 2) / fs->bytes_per_sector); // entry is in this sector of the FAT
        uint32_t in_sector = regions.fat_region_start + in_fat_sector; // entry is in this sector of the disk
        uint8_t* fat_sector = stdmem_allocate(fs->bytes_per_sector); // buffer to read the sector into
        disk_rd(in_sector, fat_sector, fs->bytes_per_sector);

        // Update the entry and write it back to disk
        uint16_t index_in_sector = (cluster_number * 2) - (in_fat_sector*fs->bytes_per_sector); // entry is at this byte index of the FAT sector
        fat_sector[index_in_sector] = successor_cluster_number & 0xFF;
        fat_sector[index_in_sector + 1] = (successor_cluster_number >> 8) & 0xFF;
        disk_wr(in_sector, fat_sector, fs->bytes_per_sector);

        return 0;
}

/**
 * Searches for the first free cluster in the FAT. If none is found, return .
 */
uint16_t find_first_free_cluster(fat16_t* fs) {
        fat16_regions_t regions = calculate_regions(fs);

        uint8_t* fat_sector = stdmem_allocate(fs->bytes_per_sector); // buffer to read the sector into

        uint16_t entries_per_sector = fs->bytes_per_sector / 2;

        for (int i = 0; i < fs->num_sectors_per_file_allocation_table; i++) {
                disk_rd(regions.fat_region_start + i, fat_sector, fs->bytes_per_sector);
                for (int j = 0; j < entries_per_sector; j++) {
                        if (fat_sector[j*2] == 0x00 && fat_sector[j*2+1] == 0x00) {
                                return entries_per_sector*i + j;
                        }
                }
        }

        return 0;
}

/**
 * Finds the byte index of the first free space in the directory cluster provided.
 * If there is no free slot, return -1
 */
int32_t find_first_free_space_in_directory_cluster(fat16_t* fs, uint8_t* cluster) {
        uint32_t bytes_per_cluster = fs->bytes_per_sector * fs->sectors_per_cluster;
        uint32_t num_entries = bytes_per_cluster / 32;

        for (int i = 0; i < num_entries; i++) {
                if (cluster[i*32] == 0x00 || cluster[i*32] == 0xe5) { // Either empty or deleted
                        return i*32;
                }
        }
        return -1;
}

int32_t find_first_free_space_in_root_directory_sector(fat16_t* fs, uint8_t* sector) {
        uint32_t num_entries = fs->bytes_per_sector / 32;

        for (int i = 0; i < num_entries; i++) {
                if (sector[i*32] == 0x00 || sector[i*32] == 0xe5) { // Either empty or deleted
                        return i*32;
                }
        }
        return -1;
}

/**
 * Finds the specified file in some cluster data and returns the byte index of
 * the beginning of the file's entry. Returns -1 if the file is not found.
 */
int32_t find_file_in_directory_cluster(fat16_t* fs, uint8_t* cluster, char* filename, char* extension) {
        uint32_t bytes_per_cluster = fs->bytes_per_sector * fs->sectors_per_cluster;
        uint32_t num_entries = bytes_per_cluster / 32;

        for (int i = 0; i < num_entries; i++) {
                bool matches = 1;
                // Compare the filename
                for (int j = 0; j < 8; j++) {
                        if (cluster[i*32 + j] == 0x20 && filename[j] == '\0') {
                                break;
                        }
                        matches &= (cluster[i*32 + j] == filename[j]);
                }
                // Compare the extension
                for (int j = 0; j < 3; j++) {
                        matches &= (cluster[i*32 + 0x8 + j] == extension[j]);
                }
                // If the match flag is still true, return the byte index
                if (matches) {
                        return i*32;
                }
        }
        return -1;
}

/**
 * Finds the specified file in some sector data and returns the byte index of
 * the beginning of the file's entry. Returns -1 if the file is not found.
 */
int32_t find_file_in_root_directory_sector(fat16_t* fs, uint8_t* sector, char* filename, char* extension) {
        uint32_t num_entries = fs->bytes_per_sector / 32;

        for (int i = 0; i < num_entries; i++) {
                bool matches = 1;
                // Compare the filename
                for (int j = 0; j < 8; j++) {
                        if (sector[i*32 + j] == 0x20 && filename[j] == '\0') {
                                break;
                        }
                        matches &= (sector[i*32 + j] == filename[j]);
                }
                // Compare the extension
                for (int j = 0; j < 3; j++) {
                        matches &= (sector[i*32 + 0x8 + j] == extension[j]);
                }
                // If the match flag is still true, return the byte index
                if (matches) {
                        return i*32;
                }
        }
        return -1;
}

void load_cluster(fat16_t* fs, uint16_t cluster_number, uint8_t* buf) {
        fat16_regions_t regions = calculate_regions(fs);

        // N.B. cluster numbers start from 2 - clusters 0 and 1 do not exist on disk
        uint32_t first_sector = regions.data_region_start + (cluster_number - 2) * fs->sectors_per_cluster;

        for (int i = 0; i < fs->sectors_per_cluster; i++) {
                disk_rd(first_sector + i, &buf[i*fs->bytes_per_sector], fs->bytes_per_sector);
        }
}

void write_cluster(fat16_t* fs, uint16_t cluster_number, uint8_t* buf) {
        fat16_regions_t regions = calculate_regions(fs);

        uint32_t first_sector = regions.data_region_start + (cluster_number - 2) * fs->sectors_per_cluster;

        for (int i = 0; i < fs->sectors_per_cluster; i++) {
                disk_wr(first_sector + i, &buf[i*fs->bytes_per_sector], fs->bytes_per_sector);
        }
}

int32_t add_root_directory_entry(fat16_t* fs, fat16_dir_entry_t* entry) {
        uint8_t* current_sector = stdmem_allocate(fs->bytes_per_sector); // buffer to read the cluster into

        fat16_regions_t regions = calculate_regions(fs);

        // Load the first cluster of the directory
        uint16_t current_sector_num = regions.root_directory_region_start;
        disk_rd(current_sector_num, current_sector, fs->bytes_per_sector);

        int32_t first_space = find_first_free_space_in_root_directory_sector(fs, current_sector);
        while (first_space == -1) {
                // Get the next sector in the directory and load it
                current_sector_num++;

                if (current_sector_num > regions.root_directory_region_start + regions.root_directory_region_size) {
                        // There is no space left in the root directory
                        return -1;
                }

                disk_rd(current_sector_num, current_sector, fs->bytes_per_sector);
                // Look for a free space in this cluster
                first_space = find_first_free_space_in_root_directory_sector(fs, current_sector);
        }

        current_sector[first_space] = entry->filename[0] == 0xe5 ? 0x05 : entry->filename[0];
        for (int i = 1; i < 8; i++) {
                current_sector[first_space + i] = entry->filename[i];
        }
        for (int i = 0; i < 3; i++) {
                current_sector[first_space + 0x08 + i] = entry->extension[i];
        }
        current_sector[first_space + 0x0b] = entry->attributes;

        current_sector[first_space + 0x1a] = (entry->first_cluster) & 0xFF;
        current_sector[first_space + 0x1b] = ((entry->first_cluster)>>8) & 0xFF;

        current_sector[first_space + 0x1c] = (entry->file_size_bytes) & 0xFF;
        current_sector[first_space + 0x1d] = ((entry->file_size_bytes)>>8) & 0xFF;
        current_sector[first_space + 0x1e] = ((entry->file_size_bytes)>>16) & 0xFF;
        current_sector[first_space + 0x1f] = ((entry->file_size_bytes)>>24) & 0xFF;

        disk_wr(current_sector_num, current_sector, fs->bytes_per_sector);

        return 0;

}

int32_t add_directory_entry(fat16_t* fs, uint16_t dir_cluster, fat16_dir_entry_t* entry) {
        uint8_t* current_cluster = stdmem_allocate(fs->bytes_per_sector * fs->sectors_per_cluster); // buffer to read the cluster into

        // Load the first cluster of the directory
        load_cluster(fs, dir_cluster, current_cluster);

        uint16_t current_cluster_num = dir_cluster;
        uint16_t previous_cluster_num = 0;
        int32_t first_space = find_first_free_space_in_directory_cluster(fs, current_cluster);
        while (first_space == -1) {
                // Get the next cluster in this directory and load it
                previous_cluster_num = current_cluster_num;
                current_cluster_num = get_successor_cluster(fs, current_cluster_num);

                if (current_cluster_num < 0x0002 || current_cluster_num > 0x0fef) {
                        // We did not find a free before the end of the existing directory clusters
                        current_cluster_num = find_first_free_cluster(fs);
                        if (current_cluster_num < 2) {
                                // We failed to find a new cluster, so report failure
                                return -1;
                        }
                        set_successor_cluster(fs, previous_cluster_num, current_cluster_num);
                        first_space = 0;
                        break;
                }

                load_cluster(fs, current_cluster_num, current_cluster);
                // Look for a free space in this cluster
                first_space = find_first_free_space_in_directory_cluster(fs, current_cluster);
        }

        current_cluster[first_space] = entry->filename[0] == 0xe5 ? 0x05 : entry->filename[0];
        for (int i = 1; i < 8; i++) {
                current_cluster[first_space + i] = entry->filename[i];
        }
        for (int i = 0; i < 3; i++) {
                current_cluster[first_space + 0x08 + i] = entry->extension[i];
        }
        current_cluster[first_space + 0x0b] = entry->attributes;

        current_cluster[first_space + 0x1a] = (entry->first_cluster) & 0xFF;
        current_cluster[first_space + 0x1b] = ((entry->first_cluster)>>8) & 0xFF;

        current_cluster[first_space + 0x1c] = (entry->file_size_bytes) & 0xFF;
        current_cluster[first_space + 0x1d] = ((entry->file_size_bytes)>>8) & 0xFF;
        current_cluster[first_space + 0x1e] = ((entry->file_size_bytes)>>16) & 0xFF;
        current_cluster[first_space + 0x1f] = ((entry->file_size_bytes)>>24) & 0xFF;

        write_cluster(fs, current_cluster_num, current_cluster);

        return 0;
}

int32_t update_root_directory_entry(fat16_t* fs, fat16_dir_entry_t* updated_entry, char* old_filename, char* old_extension) {
        uint8_t* current_sector = stdmem_allocate(fs->bytes_per_sector); // buffer to read the cluster into

        fat16_regions_t regions = calculate_regions(fs);

        // Load the first cluster of the directory
        uint16_t current_sector_num = regions.root_directory_region_start;
        disk_rd(current_sector_num, current_sector, fs->bytes_per_sector);

        int32_t file_index = find_file_in_root_directory_sector(fs, current_sector, old_filename, old_extension);
        while (file_index == -1) {
                // Get the next sector in the directory and load it
                current_sector_num++;

                if (current_sector_num > regions.root_directory_region_start + regions.root_directory_region_size) {
                        // There is no space left in the root directory
                        return -1;
                }

                disk_rd(current_sector_num, current_sector, fs->bytes_per_sector);
                // Look for a free space in this cluster
                file_index = find_file_in_root_directory_sector(fs, current_sector, old_filename, old_extension);
        }

        current_sector[file_index] = updated_entry->filename[0];
        for (int i = 1; i < 8; i++) {
                current_sector[file_index + i] = updated_entry->filename[i];
        }
        for (int i = 0; i < 3; i++) {
                current_sector[file_index + 0x08 + i] = updated_entry->extension[i];
        }
        current_sector[file_index + 0x0b] = updated_entry->attributes;

        current_sector[file_index + 0x1a] = (updated_entry->first_cluster) & 0xFF;
        current_sector[file_index + 0x1b] = ((updated_entry->first_cluster)>>8) & 0xFF;

        current_sector[file_index + 0x1c] = (updated_entry->file_size_bytes) & 0xFF;
        current_sector[file_index + 0x1d] = ((updated_entry->file_size_bytes)>>8) & 0xFF;
        current_sector[file_index + 0x1e] = ((updated_entry->file_size_bytes)>>16) & 0xFF;
        current_sector[file_index + 0x1f] = ((updated_entry->file_size_bytes)>>24) & 0xFF;

        disk_wr(current_sector_num, current_sector, fs->bytes_per_sector);

        return 0;
}

// TODO: combine with add_directory_entry?
int32_t update_directory_entry(fat16_t* fs, uint16_t dir_cluster, fat16_dir_entry_t* updated_entry, char* old_filename, char* old_extension) {
        uint8_t* current_cluster = stdmem_allocate(fs->bytes_per_sector * fs->sectors_per_cluster); // buffer to read the cluster into

        // Load the first cluster of the directory
        load_cluster(fs, dir_cluster, current_cluster);

        uint16_t current_cluster_num = dir_cluster;
        uint16_t previous_cluster_num = 0;
        int32_t file_index = find_file_in_directory_cluster(fs, current_cluster, old_filename, old_extension);
        while (file_index == -1) {
                // Get the next cluster in this directory and load it
                previous_cluster_num = current_cluster_num;
                current_cluster_num = get_successor_cluster(fs, current_cluster_num);

                if (current_cluster_num < 0x0002 || current_cluster_num > 0x0fef) {
                        // We did not find the filename we were looking for anywhere in the directory
                        return -1;
                }

                load_cluster(fs, current_cluster_num, current_cluster);
                file_index = find_file_in_directory_cluster(fs, current_cluster, old_filename, old_extension);
        }

        current_cluster[file_index] = updated_entry->filename[0];
        for (int i = 1; i < 8; i++) {
                current_cluster[file_index + i] = updated_entry->filename[i];
        }
        for (int i = 0; i < 3; i++) {
                current_cluster[file_index + 0x08 + i] = updated_entry->extension[i];
        }
        current_cluster[file_index + 0x0b] = updated_entry->attributes;

        current_cluster[file_index + 0x1a] = (updated_entry->first_cluster) & 0xFF;
        current_cluster[file_index + 0x1b] = ((updated_entry->first_cluster)>>8) & 0xFF;

        current_cluster[file_index + 0x1c] = (updated_entry->file_size_bytes) & 0xFF;
        current_cluster[file_index + 0x1d] = ((updated_entry->file_size_bytes)>>8) & 0xFF;
        current_cluster[file_index + 0x1e] = ((updated_entry->file_size_bytes)>>16) & 0xFF;
        current_cluster[file_index + 0x1f] = ((updated_entry->file_size_bytes)>>24) & 0xFF;

        write_cluster(fs, current_cluster_num, current_cluster);

        return 0;
}

fat16_file_data_t load_file(fat16_t* fs, fat16_dir_entry_t* entry) {
        uint8_t* file_data = stdmem_allocate(entry->file_size_bytes);
        uint32_t file_clusters_written = 0;
        uint16_t next_cluster = entry->first_cluster;

        while (0x0001 < next_cluster && next_cluster < 0xfff0) {
                load_cluster(fs, next_cluster, &file_data[file_clusters_written * fs->sectors_per_cluster * fs->bytes_per_sector]);
                file_clusters_written++;
                next_cluster = get_successor_cluster(fs, next_cluster);
        }

        fat16_file_data_t file;
        file.data = file_data;
        file.num_bytes = file_clusters_written * fs->sectors_per_cluster * fs->bytes_per_sector;

        return file;
}

int32_t read_from_file(fat16_t* fs, fat16_dir_entry_t* entry, uint32_t offset_bytes, char* buf, uint32_t num_bytes) {
        uint32_t bytes_per_cluster = fs->bytes_per_sector * fs->sectors_per_cluster;
        uint16_t data_starts_in_cluster = offset_bytes / bytes_per_cluster;
        uint16_t data_offset_in_cluster = offset_bytes - (data_starts_in_cluster * bytes_per_cluster);

        uint8_t* current_cluster_data = stdmem_allocate(fs->sectors_per_cluster * fs->bytes_per_sector);

        uint32_t current_cluster_index = 0; // The index of this cluster chain that we are at
        uint16_t current_cluster_num = entry->first_cluster; // The cluster number (wrt the filesystem)

        uint32_t num_bytes_read = 0;

        while (0x0001 < current_cluster_num && current_cluster_num < 0xfff0 && current_cluster_index < data_starts_in_cluster) {
                current_cluster_num = get_successor_cluster(fs, current_cluster_num);
                current_cluster_index++;
        }

        load_cluster(fs, current_cluster_num, current_cluster_data);

        uint32_t num_bytes_to_copy_from_first_cluster = (bytes_per_cluster - data_offset_in_cluster) > num_bytes ? num_bytes : bytes_per_cluster - data_offset_in_cluster;
        stdmem_copy(buf, &current_cluster_data[data_offset_in_cluster], num_bytes_to_copy_from_first_cluster);
        num_bytes_read += num_bytes_to_copy_from_first_cluster;

        current_cluster_num = get_successor_cluster(fs, current_cluster_num);
        current_cluster_index++;

        while (0x0001 < current_cluster_num && current_cluster_num < 0xfff0 && num_bytes_read < num_bytes) {
                load_cluster(fs, current_cluster_num, current_cluster_data);

                uint32_t num_bytes_to_copy_from_cluster = (num_bytes - num_bytes_read) > bytes_per_cluster ? bytes_per_cluster : (num_bytes - num_bytes_read);
                stdmem_copy(&buf[num_bytes_read], current_cluster_data, num_bytes_to_copy_from_cluster);
                num_bytes_read += num_bytes_to_copy_from_cluster;

                current_cluster_num = get_successor_cluster(fs, current_cluster_num);
                current_cluster_index++;
        }

        return num_bytes_read;
}

// TODO: expand the file if we run out of space
int32_t write_to_file(fat16_t* fs, fat16_dir_entry_t* entry, uint32_t offset_bytes, char* buf, uint32_t num_bytes) {
        uint32_t bytes_per_cluster = fs->bytes_per_sector * fs->sectors_per_cluster;
        uint16_t data_starts_in_cluster = offset_bytes / bytes_per_cluster;
        uint16_t data_offset_in_cluster = offset_bytes - (data_starts_in_cluster * bytes_per_cluster);

        uint8_t* current_cluster_data = stdmem_allocate(fs->sectors_per_cluster * fs->bytes_per_sector);

        uint32_t current_cluster_index = 0; // The index of this cluster chain that we are at
        uint16_t previous_cluster_num = 0;
        uint16_t current_cluster_num = entry->first_cluster; // The cluster number (wrt the filesystem)

        uint32_t num_bytes_written = 0;

        while (0x0001 < current_cluster_num && current_cluster_num < 0xfff0 && current_cluster_index < data_starts_in_cluster) {
                previous_cluster_num = current_cluster_num;
                current_cluster_num = get_successor_cluster(fs, current_cluster_num);
                current_cluster_index++;
        }

        load_cluster(fs, current_cluster_num, current_cluster_data);

        uint32_t num_bytes_to_write_to_first_cluster = (bytes_per_cluster - data_offset_in_cluster) > num_bytes ? num_bytes : bytes_per_cluster - data_offset_in_cluster;
        stdmem_copy(&current_cluster_data[data_offset_in_cluster], buf, num_bytes_to_write_to_first_cluster);
        write_cluster(fs, current_cluster_num, current_cluster_data);
        num_bytes_written += num_bytes_to_write_to_first_cluster;

        previous_cluster_num = current_cluster_num;
        current_cluster_num = get_successor_cluster(fs, current_cluster_num);
        current_cluster_index++;

        while (0x0001 < current_cluster_num && current_cluster_num < 0xfff0 && num_bytes_written < num_bytes) {
                load_cluster(fs, current_cluster_num, current_cluster_data);

                uint32_t num_bytes_to_write_to_cluster = (num_bytes - num_bytes_written) > bytes_per_cluster ? bytes_per_cluster : (num_bytes - num_bytes_written);
                stdmem_copy(current_cluster_data, &buf[num_bytes_written], num_bytes_to_write_to_cluster);
                write_cluster(fs, current_cluster_num, current_cluster_data);
                num_bytes_written += num_bytes_to_write_to_cluster;

                previous_cluster_num = current_cluster_num;
                current_cluster_num = get_successor_cluster(fs, current_cluster_num);
                current_cluster_index++;
        }

        while (num_bytes_written < num_bytes) {
                uint16_t next_free_cluster_num = find_first_free_cluster(fs);
                if (next_free_cluster_num < 2) {
                        return num_bytes_written;
                }
                set_successor_cluster(fs, previous_cluster_num, next_free_cluster_num);
                set_successor_cluster(fs, next_free_cluster_num, 0xFFFF);

                current_cluster_num = next_free_cluster_num;
                load_cluster(fs, current_cluster_num, current_cluster_data);

                uint32_t num_bytes_to_write_to_cluster = (num_bytes - num_bytes_written) > bytes_per_cluster ? bytes_per_cluster : (num_bytes - num_bytes_written);
                stdmem_copy(current_cluster_data, &buf[num_bytes_written], num_bytes_to_write_to_cluster);
                write_cluster(fs, current_cluster_num, current_cluster_data);
                num_bytes_written += num_bytes_to_write_to_cluster;
                previous_cluster_num = current_cluster_num;
        }

        return num_bytes_written;
}

uint16_t num_clusters_in_chain(fat16_t* fs, uint16_t first_cluster) {
        uint16_t count = 0;

        if (0x0001 < first_cluster && first_cluster < 0xfff0) {
                count++;

                uint16_t next_cluster = get_successor_cluster(fs, first_cluster);
                while (0x0001 < next_cluster && next_cluster < 0xfff0) {
                        count++;
                        next_cluster = get_successor_cluster(fs, next_cluster);
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
                                list_entry->entry.extension[k] = sector[entry_bytes*j + 0x08 + k] == 0x20 ? '\0' : sector[entry_bytes*j + 0x08 + k];
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

                uint16_t next_cluster = get_successor_cluster(fs, entry->first_cluster);

                while (0x0001 < next_cluster && next_cluster < 0xfff0 && !entries_have_terminated) {
                        load_cluster(fs, next_cluster, current_cluster);

                        for (int i = 0; i < fs->sectors_per_cluster && !entries_have_terminated; i++) {
                                entries_have_terminated = add_sector_dir_entries_to_list(fs, &current_cluster[i*fs->bytes_per_sector], result);
                        }

                        next_cluster = get_successor_cluster(fs, next_cluster);
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

tailq_open_file_t* find_open_file(char* pathname) {
        tailq_open_file_t* open_file;
        TAILQ_FOREACH(open_file, open_files, entries) {
                if (stdstring_compare(open_file->path, pathname) == 0) {
                        return open_file;
                }
        }
        return NULL;
}

tailq_open_file_t* find_open_file_by_fd(filedesc_t fd) {
        tailq_open_file_t* open_file;
        TAILQ_FOREACH(open_file, open_files, entries) {
                if (open_file->fd == fd) {
                        return open_file;
                }
        }
        return NULL;
}

fat16_file_path_t split_path(char* pathname) {
        if (pathname[0] == '/') {
                pathname = &pathname[1];
        }

        if (pathname[1] == '\0') {
                return (fat16_file_path_t){NULL, 0};
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
        bool reached_terminator = 0;
        while (!reached_terminator) {
                int j = 0;
                while (pathname[i+j] != '/' && pathname[i+j] != '\0') {
                        j++;
                }

                if (pathname[i+j] == '\0') {
                        reached_terminator = 1;
                }

                parts[part_num] = stdmem_allocate(sizeof(char)*(j+1));

                stdmem_copy(&(parts[part_num][0]), &pathname[i], j);
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
        fat16_file_name_t filename = split_filename(path.parts[0]);

        if (path.num_parts > 1) {
                // Search for next directory in the path
                tailq_fat16_dir_entry_t* dir_entry;
                TAILQ_FOREACH(dir_entry, dir, entries) {
                        fat16_file_attr_t attributes = unpack_file_attributes(dir_entry->entry.attributes);
                        if (attributes.is_subdirectory) {
                                if (stdstring_compare(filename.name, &(dir_entry->entry.filename[0])) == 0 && stdstring_compare(filename.extension, &(dir_entry->entry.extension[0])) == 0) {
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
                TAILQ_FOREACH(file_entry, dir, entries) {
                        fat16_file_attr_t attributes = unpack_file_attributes(file_entry->entry.attributes);
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
        } else if (pathname[1] == '\0') {
                // If we are opening the root directory - bit of a hack :s
                fat16_dir_entry_t* root = stdmem_allocate(sizeof(fat16_dir_entry_t));
                root->filename[0] = '/';
                root->filename[1] = '\0';
                root->extension[0] = '\0';
                root->attributes = 0x10;
                root->first_cluster = 0;
                fat16_regions_t regions = calculate_regions(fs);
                root->file_size_bytes = regions.root_directory_region_size * fs->bytes_per_sector; // Should really be calculated from number of entries

                tailq_open_file_t* opened_root = stdmem_allocate(sizeof(tailq_open_file_t));
                opened_root->directory_entry = root;
                opened_root->fd = get_next_filedesc();
                opened_root->offset = 0;
                opened_root->path = pathname;
                TAILQ_INSERT_TAIL(open_files, opened_root, entries);
                return opened_root;
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
 * -1 = failure, 0 = success
 */
int32_t create_new_file(char* pathname, uint8_t attributes) {
        // Create the file entry
        char* filename = to_filename(pathname);
        fat16_file_name_t file = split_filename(filename);

        fat16_dir_entry_t* new_file = stdmem_allocate(sizeof(fat16_dir_entry_t));
        int i;
        for (i = 0; i < 8 && file.name[i] != '\0'; i++) {
                new_file->filename[i] = file.name[i];
        }
        for (; i < 8; i++) {
                new_file->filename[i] = 0x20;
        }

        for (i = 0; i < 3 && file.extension[i] != '\0'; i++) {
                new_file->extension[i] = file.extension[i];
        }
        for (; i < 3; i++) {
                new_file->extension[i] = 0x20;
        }
        new_file->attributes = attributes;
        new_file->first_cluster = find_first_free_cluster(fs);
        new_file->file_size_bytes = 0;

        // Add it to correct directory
        tailq_fat16_dir_head_t* root_dir = get_root_dir(fs);
        char* dir_path = to_directory(pathname);
        if (dir_path[0] == '\0') {
                // Add to root folder
                int32_t root_result = add_root_directory_entry(fs, new_file);
                if (root_result != 0) {
                        return -1;
                }
        } else {
                // Add to a non-root folder

                // If the directory doesn't exist, fail
                fat16_dir_entry_t* dir = find_dir(dir_path, root_dir);
                if (dir == NULL) {
                        return -1;
                }

                int32_t dir_result = add_directory_entry(fs, dir->first_cluster, new_file);
                if (dir_result != 0) {
                        return -1;
                }
        }

        set_successor_cluster(fs, new_file->first_cluster, 0xFFFF);

        return 0;
}

/**
 * -1 = failure, 0 = success
 */
int32_t update_file_details(char* pathname, fat16_dir_entry_t* updated_entry, char* old_filename, char* old_extension) {
        tailq_fat16_dir_head_t* root_dir = get_root_dir(fs);
        if (pathname[0] != '/') {
                return -1;
        }
        char* stripped_pathname = &pathname[1];

        // If the directory doesn't exist, fail
        char* dir_path = to_directory(stripped_pathname);
        fat16_dir_entry_t* file = find_file(stripped_pathname, root_dir);
        if (file == NULL) {
                return -1;
        }

        if (dir_path[0] == '\0') {
                // In root folder
                return update_root_directory_entry(fs, updated_entry, old_filename, old_extension);
        } else {
                // In another folder
                fat16_dir_entry_t* dir = find_dir(dir_path, root_dir);
                if (dir == NULL) {
                        return -1;
                }

                //char* filename = to_filename(stripped_pathname);
                //fat16_file_name_t file = split_filename(filename);

                return update_directory_entry(fs, dir->first_cluster, updated_entry, old_filename, old_extension);
        }
}

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
                                if (-1 == create_new_file(pathname, 0x0)) {
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
                result->append = 1;
        } else {
                result->append = 0;
        }

        return result->fd;
}

int32_t sys_write(int fd, char *buf, size_t nbytes) {
        // If printing to stdout
        if (STDOUT_FILEDESC == fd) {
                int32_t i;
                for (i = 0; i < nbytes; i++) {
                        PL011_putc(UART0, *buf++);
                }
                return i;
        } else {
                tailq_open_file_t* open_file = find_open_file_by_fd(fd);
                if (open_file == NULL) {
                        return -1;
                } else {
                        fat16_file_attr_t attributes = unpack_file_attributes(open_file->directory_entry->attributes);
                        if (attributes.is_subdirectory) {
                                return -1; // Cannot write directly to a directory
                        }
                        // If the append flag is set, start writing at the end of the file. Else start writing at the current offset.
                        if (open_file->append) {
                                open_file->offset = open_file->directory_entry->file_size_bytes;
                        }
                        int32_t num_bytes_written = write_to_file(fs, open_file->directory_entry, open_file->offset, buf, nbytes);
                        if (num_bytes_written < 0) {
                                return -1;
                        }
                        // If we have increased the size of the file, update the file metadata and write it to disk
                        if (open_file->offset + num_bytes_written > open_file->directory_entry->file_size_bytes) {
                                open_file->directory_entry->file_size_bytes = open_file->offset + num_bytes_written;
                                update_file_details(open_file->path, open_file->directory_entry, &open_file->directory_entry->filename[0], &open_file->directory_entry->extension[0]);
                        }

                        open_file->offset += num_bytes_written;
                        return num_bytes_written;
                }
        }
}

int32_t sys_read(filedesc_t fd, char *buf, size_t nbytes) {
        // If reading from stdin
        if (STDIN_FILEDESC == fd) {
                int32_t i = 0;
                while (i < nbytes) {
                        buf[i] = stdstream_pop_char(stdin_buffer);
                        i++;
                }
                return i;
        } else {
                tailq_open_file_t* open_file = find_open_file_by_fd(fd);
                if (open_file == NULL) {
                        return -1;
                } else {
                        if (open_file->directory_entry->first_cluster < 2) {
                                // This is the root directory - don't allow direct reads
                                // because the static sector allocation makes this really complicated
                                return -1;
                        }
                        int32_t num_bytes_read = read_from_file(fs, open_file->directory_entry, open_file->offset, buf, nbytes);
                        if (num_bytes_read < 0) {
                                return -1;
                        }
                        open_file->offset += num_bytes_read;
                        return num_bytes_read;
                }
        }
}

int32_t sys_close(filedesc_t fd) {
        tailq_open_file_t* file = find_open_file_by_fd(fd);
        if (file == NULL) {
                return -1;
        } else {
                TAILQ_REMOVE(open_files, file, entries);
                return 0;
        }
}

// TODO: meaningful errors
int32_t sys_unlink(char* pathname) {
        tailq_open_file_t* open_file = find_open_file(pathname);
        if (open_file != NULL) {
                // Cannot delete an open file
                return -1;
        }

        if (pathname[0] != '/') {
                return -1;
        }

        fat16_dir_entry_t* file = find_file(&pathname[1], get_root_dir(fs));
        if (file == NULL) {
                return -1;
        }

        char* old_filename = stdmem_allocate(9);
        stdmem_copy(old_filename, &file->filename[0], 9);

        file->filename[0] = 0xe5;

        update_file_details(pathname, file, old_filename, &file->extension[0]);
        return 0;
}

int32_t sys_lseek(filedesc_t fd, int32_t offset, int32_t whence) {
        tailq_open_file_t* file = find_open_file_by_fd(fd);
        if (file == NULL) {
                return -1;
        } else {
                switch (whence) {
                        case SEEK_SET : {
                                file->offset = offset;
                                return 0;
                        }
                        case SEEK_CUR : {
                                file->offset += offset;
                                return 0;
                        }
                        case SEEK_END : {
                                file->offset = file->directory_entry->file_size_bytes + offset;
                                return 0;
                        }
                        default : {
                                return -1;
                        }
                }
        }
}

tailq_fat16_dir_head_t* sys_getdents(filedesc_t fd, int32_t max_num) {
        tailq_open_file_t* open_directory = find_open_file_by_fd(fd);
        if (open_directory == NULL) {
                return NULL;
        }

        // If this is the root directory
        if (open_directory->directory_entry->first_cluster < 2) {
                return get_root_dir(fs);
        }

        return get_dir(fs, open_directory->directory_entry);
}

char* current_working_dir;
int32_t sys_chdir(char* path) {
        int32_t len = stdstring_length(path);
        if (stdstring_compare("/", path) == 0) {
                current_working_dir[0] = '/';
                current_working_dir[1] = '\0';
                return 0;
        } else if (path[0] == '/'&& find_dir(path, get_root_dir(fs)) != NULL) {
                stdmem_copy(current_working_dir, path, len + 1);
                return 0;
        } else if (find_dir(path, get_dir(fs, find_dir(current_working_dir, get_root_dir(fs))))) {
                // TODO: not currently working
                size_t start = stdstring_length(current_working_dir);
                if (start < 100) {
                        current_working_dir[start] = '/';
                }
                stdmem_copy(&current_working_dir[start+1], path, 100-(start+1));
                return 0;
        } else {
                return -1;
        }
}

char* sys_getcwd(char* buf, size_t nbytes) {
        size_t cwd_len = stdstring_length(current_working_dir);
        size_t len = cwd_len > nbytes ? nbytes : cwd_len;
        stdmem_copy(buf, current_working_dir, len);
        return buf;
}

int32_t sys_stat(char* path, fat16_dir_entry_t* buf) {
        fat16_dir_entry_t* file = find_file(path, get_root_dir(fs));

        if (file != NULL) {
                stdmem_copy(buf, file, sizeof(fat16_dir_entry_t));
                return 0;
        }

        return -1;
}

int32_t sys_fstat(filedesc_t fd, fat16_dir_entry_t* buf) {
        tailq_open_file_t* file = find_open_file_by_fd(fd);

        if (file != NULL) {
                stdmem_copy(buf, file->directory_entry, sizeof(fat16_dir_entry_t));
                return 0;
        }
        return -1;
}

int32_t sys_mkdir(char* path) {
        tailq_fat16_dir_head_t* root_dir = get_root_dir(fs);
        if (path[0] != '/') {
                return -1;
        } else if (path[1] == '\0') {
                return -1;
        } else {
                fat16_dir_entry_t* file = find_file(&path[1], root_dir);
                // If the file already exists:
                if (file != NULL) {
                        return -1;
                }
        }
        return create_new_file(path, 0x10);
}

/**
 * Initialise the various streams and the filesystem.
 */
void file_initialise() {
        stdin_buffer = stdstream_initialise_buffer();
        stdout_buffer = stdstream_initialise_buffer();
        stderr_buffer = stdstream_initialise_buffer();
        fs = inspect_file_system();
        TAILQ_INIT(open_files);
        current_working_dir = stdmem_allocate(sizeof(char)*100);
        sys_chdir("/");
}
