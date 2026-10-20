#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint16_t block_size;
    uint32_t block_count;
    uint32_t fat_start;
    uint32_t fat_blocks;
    uint32_t root_dir_start;
    uint32_t root_dir_blocks;
} SuperBlock;

typedef struct {
    uint8_t status;
    uint32_t starting_block;
    uint32_t num_blocks;
    uint32_t file_size;
    char creation_time[7];
    char modification_time[7];
    char filename[32];
    char unused[6];
} DirectoryEntry;

#define DIR_ENTRY_SIZE 64

int read_superblock(FILE *fp, SuperBlock *sb);
void parse_directory_entry(unsigned char buffer[], DirectoryEntry *entry);
int find_directory(FILE *input,
                   uint32_t start_block,
                   uint32_t number_of_blocks,
                   uint32_t block_size,
                   char *name,
                   DirectoryEntry *found);

int navigate_path(FILE *input,
                  SuperBlock *sb,
                  char *path,
                  DirectoryEntry *result);

int find_file(FILE *input,
               uint32_t start_block,
               uint32_t number_of_blocks,
               uint32_t block_size,
               char *name,
               DirectoryEntry *found);

#endif