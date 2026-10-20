#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h> 
#include "filesystem.h"

char get_entry_type(uint8_t status);
void print_creation_time(char time[7]);
void list_directory(FILE *input, uint32_t start_block, uint32_t number_of_blocks, uint32_t block_size);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./disklist image /sub_dir\n");
        return 1;
    }

    FILE *input = fopen(argv[1], "rb");

    if (input == NULL) {
        fprintf(stderr, "Could not open %s.\n", argv[1]);
        return 1;
    }

    SuperBlock sb;

    if (!read_superblock(input, &sb)) {
        fprintf(stderr, "Error: could not read superblock.\n");
        fclose(input);
        return 1;
    }

    // handle root directory "/"
    if (strcmp(argv[2], "/") == 0) {

        list_directory(input,
                    sb.root_dir_start,
                    sb.root_dir_blocks,
                    sb.block_size);

    } else {

        DirectoryEntry directory;

        if (navigate_path(input,
                        &sb,
                        argv[2],
                        &directory)) {

            list_directory(input,
                        directory.starting_block,
                        directory.num_blocks,
                        sb.block_size);

        } else {
            fprintf(stderr, "Path not found: %s\n", argv[2]);
        }
    }


    fclose(input);
    return 0;
}

char get_entry_type(uint8_t status) {
    if (status & 0x02) {
        return 'F';
    }

    if (status & 0x04) {
        return 'D';
    }

    return '?'; // unknown type
} // get_entry_type

void print_creation_time(char time[7]) {
    uint16_t year;

    memcpy(&year, time, 2);
    year = ntohs(year);

    printf("%04u/%02u/%02u %02u:%02u:%02u",
           year,
           (unsigned char)time[2],
           (unsigned char)time[3],
           (unsigned char)time[4],
           (unsigned char)time[5],
           (unsigned char)time[6]);
} // print_creation_time

void list_directory(FILE *input, uint32_t start_block, uint32_t number_of_blocks, uint32_t block_size) {
    uint64_t directory_offset = (uint64_t)start_block * block_size;

    if (fseek(input, directory_offset, SEEK_SET) != 0) {
        fprintf(stderr, "Failed to seek to directory.\n");
        return;
    }

    unsigned char buffer[DIR_ENTRY_SIZE];
    DirectoryEntry entry;

    int total_entries = (number_of_blocks * block_size) / DIR_ENTRY_SIZE;

    for (int i = 0; i < total_entries; i++) {
        if (fread(buffer, DIR_ENTRY_SIZE, 1, input) != 1) {
            fprintf(stderr, "Failed to read directory entry.\n");
            return;
        }

        parse_directory_entry(buffer, &entry);

        if (entry.status & 0x01) {
            char type = get_entry_type(entry.status);

            printf("%c %10u %30s ",
                   type,
                   entry.file_size,
                   entry.filename);

            print_creation_time(entry.creation_time);
            printf("\n");
        }
    }
} // list_directory 



