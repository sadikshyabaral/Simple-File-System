#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

//#include "superblock.h"
#include "filesystem.h"

uint32_t get_fat_entry(FILE *input,
                       SuperBlock *sb,
                       uint32_t block_number);

void copy_file(FILE *input,
               FILE *output,
               SuperBlock *sb,
               DirectoryEntry *file);

int main(int argc, char *argv[]) {
    // Check for exactly one command-line argument
    if (argc != 4) {
        fprintf(stderr, "Usage: ./diskget image /path/to/file output\n");

        return 1;
    }

    // Open the image file in binary read mode
    FILE *input = fopen(argv[1], "rb");

    // Exit with an error if the file cannot be opened
    if (input == NULL) {
        fprintf(stderr, "Could not open %s.\n", argv[1]);
        return 1;
    }

    SuperBlock sb;

    if (!read_superblock(input, &sb)) {
        fprintf(stderr, "Error reading superblock\n");
        fclose(input);
        return 1;
    }

    char path_copy[256];
    if (strlen(argv[2]) >= sizeof(path_copy))
    {
        fprintf(stderr,"Path too long\n");
        fclose(input);
        return 1;
    }

    strcpy(path_copy, argv[2]);


    char *filename = strrchr(path_copy, '/');
    char *directory_path;

    if (filename == NULL) {
        // No slash: file is in the root directory
        directory_path = "";
        filename = path_copy;
    } else {
        *filename = '\0';   // Split the path
        directory_path = path_copy;
        filename++;         // Move past '/'
    }

    DirectoryEntry directory;

    if (strcmp(directory_path, "") == 0) {

        // File is in root directory
        directory.starting_block = sb.root_dir_start;
        directory.num_blocks = sb.root_dir_blocks;

    } else {

        // File is inside a subdirectory
        if (!navigate_path(input,
                        &sb,
                        directory_path,
                        &directory)) {

            fprintf(stderr,
                    "Directory not found: %s\n",
                    directory_path);

            fclose(input);
            return 1;
        }
    }


    DirectoryEntry file;

    if (!find_file(input,
                directory.starting_block,
                directory.num_blocks,
                sb.block_size,
                filename,
                &file)) {

        fprintf(stderr,
                "Requested file %s not found in %s.\n",
                filename,
                argv[2]);


        fclose(input);
        return 1;
    }

    FILE *output = fopen(argv[3], "wb");

    if (output == NULL) {
        fprintf(stderr, "Could not create %s.\n", argv[3]);
        fclose(input);
        return 1;
    }

    copy_file(input, output, &sb, &file);

    fclose(output);
    fclose(input);
    return 0;
}

uint32_t get_fat_entry(FILE *input,
                       SuperBlock *sb,
                       uint32_t block_number) {

    // FAT starts at:
    // fat_start * block_size
    uint64_t fat_offset =
        (uint64_t)sb->fat_start * sb->block_size;

    // Each FAT entry is 4 bytes
    uint64_t entry_offset =
        fat_offset + ((uint64_t)block_number * sizeof(uint32_t));


    if (fseek(input, entry_offset, SEEK_SET) != 0) {
        fprintf(stderr, "Failed to seek FAT entry\n");
        return 0;
    }


    uint32_t next_block;

    if (fread(&next_block, sizeof(uint32_t), 1, input) != 1) {
        fprintf(stderr, "Failed to read FAT entry\n");
        return 0;
    }


    // FAT values are stored big-endian
    next_block = ntohl(next_block);

    return next_block;
} // get_fat_entry

void copy_file(FILE *input,
               FILE *output,
               SuperBlock *sb,
               DirectoryEntry *file) {

    uint32_t current_block = file->starting_block;

    uint32_t bytes_remaining = file->file_size;

    unsigned char *buffer = malloc(sb->block_size);

    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }


    while (current_block != 0xFFFFFFFF) {

        // Find block location in image
        uint64_t block_offset = (uint64_t)current_block * sb->block_size;


        if (fseek(input, block_offset, SEEK_SET) != 0) {
            fprintf(stderr, "Failed to seek data block\n");
            free(buffer);
            return;
        }


        // Read block data
        if (fread(buffer, sb->block_size, 1, input) != 1) {
            fprintf(stderr, "Failed to read data block\n");
            free(buffer);
            return;
        }


        // Only write remaining bytes
        uint32_t bytes_to_write = sb->block_size;

        if (bytes_remaining < sb->block_size) {
            bytes_to_write = bytes_remaining;
        }

        if (fwrite(buffer, bytes_to_write, 1, output) != 1) {
            fprintf(stderr,"Failed writing output file\n");
            free(buffer);
            return;
        }

        bytes_remaining -= bytes_to_write;

        // Finished copying the file
        if (bytes_remaining == 0) {
            break;
        }


        // Move to next FAT block
        current_block = get_fat_entry(input, sb, current_block);
    }


    free(buffer);
} // copy_file


