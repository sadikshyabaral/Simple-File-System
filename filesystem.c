#include "filesystem.h"

#include <string.h>
#include <arpa/inet.h>   // ntohl(), ntohs()

int read_superblock(FILE *fp, SuperBlock *sb) {
    uint8_t buffer[512];

    // Go to the beginning of the file
    if (fseek(fp, 0, SEEK_SET) != 0)
        return 0;

    // Read the superblock
    if (fread(buffer, 1, 512, fp) != 512)
        return 0;

    // Extract fields
    memcpy(&sb->block_size, buffer + 8, sizeof(uint16_t));
    sb->block_size = ntohs(sb->block_size);

    memcpy(&sb->block_count, buffer + 10, sizeof(uint32_t));
    sb->block_count = ntohl(sb->block_count);

    memcpy(&sb->fat_start, buffer + 14, sizeof(uint32_t));
    sb->fat_start = ntohl(sb->fat_start);

    memcpy(&sb->fat_blocks, buffer + 18, sizeof(uint32_t));
    sb->fat_blocks = ntohl(sb->fat_blocks);

    memcpy(&sb->root_dir_start, buffer + 22, sizeof(uint32_t));
    sb->root_dir_start = ntohl(sb->root_dir_start);

    memcpy(&sb->root_dir_blocks, buffer + 26, sizeof(uint32_t));
    sb->root_dir_blocks = ntohl(sb->root_dir_blocks);

    return 1;
}

// Read one directory entry from a 64-byte buffer
void parse_directory_entry(unsigned char buffer[], DirectoryEntry *entry) {
    entry->status = buffer[0];

    // Copy big-endian values
    memcpy(&entry->starting_block, buffer + 1, sizeof(uint32_t));
    memcpy(&entry->num_blocks, buffer + 5, sizeof(uint32_t));
    memcpy(&entry->file_size, buffer + 9, sizeof(uint32_t));

    // Convert from filesystem byte order to host byte order
    entry->starting_block = ntohl(entry->starting_block);
    entry->num_blocks = ntohl(entry->num_blocks);
    entry->file_size = ntohl(entry->file_size);

    // Copy timestamps
    memcpy(entry->creation_time, buffer + 13, 7);
    memcpy(entry->modification_time, buffer + 20, 7);

    // Copy filename
    memcpy(entry->filename, buffer + 27, 31);

    // Copy unused bytes
    memcpy(entry->unused, buffer + 58, 6);

    // Make filename printable
    entry->filename[31] = '\0';
} // parse_directory_entry

int find_directory(FILE *input,
                   uint32_t start_block,
                   uint32_t number_of_blocks,
                   uint32_t block_size,
                   char *name,
                   DirectoryEntry *found) {

    uint64_t directory_offset = (uint64_t)start_block * block_size;

    if (fseek(input, directory_offset, SEEK_SET) != 0) {
        return 0;
    }

    unsigned char buffer[DIR_ENTRY_SIZE];
    DirectoryEntry entry;

    int total_entries = (number_of_blocks * block_size) / DIR_ENTRY_SIZE;

    for (int i = 0; i < total_entries; i++) {

        if (fread(buffer, DIR_ENTRY_SIZE, 1, input) != 1) {
            return 0;
        }

        parse_directory_entry(buffer, &entry);

        if ((entry.status & 0x01) &&
            (entry.status & 0x04) &&
            strcmp(entry.filename, name) == 0) {

            *found = entry;
            return 1;
        }
    }

    return 0;
} // find_directory 

int navigate_path(FILE *input,
                  SuperBlock *sb,
                  char *path,
                  DirectoryEntry *result) {

    // Start at root directory
    uint32_t current_start = sb->root_dir_start;
    uint32_t current_blocks = sb->root_dir_blocks;

    // Copy path because strtok modifies the string
    char path_copy[256];
    strcpy(path_copy, path);

    // Remove leading '/'
    char *token = strtok(path_copy + 1, "/");

    DirectoryEntry entry;

    while (token != NULL) {

        if (!find_directory(input,
                            current_start,
                            current_blocks,
                            sb->block_size,
                            token,
                            &entry)) {

            return 0; // directory not found
        }

        // Move into the found directory
        current_start = entry.starting_block;
        current_blocks = entry.num_blocks;

        token = strtok(NULL, "/");
    }

    *result = entry;

    return 1;
} // navigate_path

int find_file(FILE *input,
              uint32_t start_block,
              uint32_t number_of_blocks,
              uint32_t block_size,
              char *name,
              DirectoryEntry *found) {

    uint64_t offset = (uint64_t)start_block * block_size;

    if (fseek(input, offset, SEEK_SET) != 0) {
        return 0;
    }

    unsigned char buffer[DIR_ENTRY_SIZE];
    DirectoryEntry entry;

    int total_entries =
        (number_of_blocks * block_size) / DIR_ENTRY_SIZE;


    for (int i = 0; i < total_entries; i++) {

        if (fread(buffer, DIR_ENTRY_SIZE, 1, input) != 1) {
            return 0;
        }

        parse_directory_entry(buffer, &entry);


        if ((entry.status & 0x01) &&
            (entry.status & 0x02) &&
            strcmp(entry.filename, name) == 0) {

            *found = entry;
            return 1;
        }
    }

    return 0;
} // find_file