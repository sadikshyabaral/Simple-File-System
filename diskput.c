#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include "filesystem.h"

uint32_t find_free_block(FILE *input, SuperBlock *sb);
int allocate_file_blocks(FILE *input, SuperBlock *sb, uint32_t blocks_needed, uint32_t *first_block);
void write_file_data(FILE *input, FILE *source,  SuperBlock *sb,  uint32_t first_block,  uint32_t file_size);
uint32_t get_fat_entry(FILE *input, SuperBlock *sb, uint32_t block_number);
void write_fat_entry(FILE *input, SuperBlock *sb, uint32_t block, uint32_t value);
DirectoryEntry create_file_entry(char *filename, uint32_t first_block, uint32_t blocks, uint32_t file_size);
int write_directory_entry(FILE *input, SuperBlock *sb,  DirectoryEntry *directory, DirectoryEntry *new_file);
void free_file_blocks(FILE *input, SuperBlock *sb, uint32_t first_block);

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr,
        "Usage: ./diskput image local_file /destination/path\n");
        return 1;
    }

    FILE *input = fopen(argv[1], "r+b");

    if (input == NULL) {
        fprintf(stderr, "Could not open %s\n", argv[1]);
        return 1;
    }

    SuperBlock sb;

    if (!read_superblock(input, &sb)) {
        fprintf(stderr, "Could not read superblock\n");
        fclose(input);
        return 1;
    }

    FILE *source = fopen(argv[2], "rb");
    if (source == NULL) {
        fprintf(stderr, "Source file %s not found.\n", argv[2]);
        fclose(input);
        return 1;
    }

    fseek(source, 0, SEEK_END);

    long size = ftell(source);
    if(size < 0) {
        fprintf(stderr,"Could not determine file size\n");
        return 1;
    }
    uint32_t file_size = (uint32_t)size;


    rewind(source);

    char path_copy[256];

    strncpy(path_copy, argv[3], sizeof(path_copy)-1);
    path_copy[sizeof(path_copy)-1] = '\0';


    char *filename = strrchr(path_copy, '/');

    if (filename == NULL) {
        fprintf(stderr,"Invalid destination path\n");
        fclose(source);
        fclose(input);
        return 1;
    }

    *filename = '\0';
    filename++;

    if(strlen(filename) > 31) {
        fprintf(stderr,"Filename too long\n");
        fclose(source);
        fclose(input);
        return 1;
    }

    char *directory_path = path_copy;

    DirectoryEntry directory;

    if (strlen(directory_path) == 0) {

        directory.starting_block = sb.root_dir_start;
        directory.num_blocks = sb.root_dir_blocks;

    } else {

        if (!navigate_path(input, &sb, directory_path,  &directory)) {
            fprintf(stderr, "Directory not found: %s\n", directory_path);

            fclose(source);
            fclose(input);
            return 1;
        }
    }

    DirectoryEntry existing;

    if(find_file(input, directory.starting_block, directory.num_blocks, sb.block_size, filename, &existing)) {
        fprintf(stderr,"File already exists\n");
        fclose(source);
        fclose(input);
        return 1;
    }

    uint32_t blocks_needed = (file_size + sb.block_size - 1)  / sb.block_size;
    uint32_t first_block;

    if(file_size == 0) {
        blocks_needed = 0;
    }

    if (!allocate_file_blocks(input, &sb, blocks_needed, &first_block)) {

        fprintf(stderr,"Not enough free blocks\n");

        fclose(source);
        fclose(input);
        return 1;
    }

    write_file_data(input, source, &sb, first_block, file_size);

    DirectoryEntry new_file = create_file_entry(filename, first_block, blocks_needed, file_size);

    if (!write_directory_entry(input, &sb, &directory, &new_file)) {

        fprintf(stderr, "Could not write directory entry\n");

        // undo FAT allocation
        if (blocks_needed > 0) {
            free_file_blocks(input, &sb, first_block);
        }

        fclose(source);
        fclose(input);
        return 1;
    }


    fclose(source);
    fclose(input);

    return 0;
}

uint32_t find_free_block(FILE *input, SuperBlock *sb) {
    uint32_t entries =
        (sb->fat_blocks * sb->block_size) / 4;


    for(uint32_t i = 0; i < entries; i++) {
        uint32_t value;

        fseek(input,
              sb->fat_start * sb->block_size + i * 4,
              SEEK_SET);

        fread(&value, 4, 1, input);

        value = ntohl(value);

        // only use truly free blocks
        if(value == 0 && i >= sb->root_dir_start + sb->root_dir_blocks){
            return i;
        }
    }
    return 0xFFFFFFFF;
}


int allocate_file_blocks(FILE *input,  SuperBlock *sb, uint32_t blocks_needed, uint32_t *first_block) {
    
    //uint32_t previous = 0xFFFFFFFF;
    //uint32_t first = 0xFFFFFFFF;

    if(blocks_needed == 0) {
        *first_block = 0xFFFFFFFF;
        return 1;
    }
    
    uint32_t *blocks = malloc(sizeof(uint32_t) * blocks_needed);
    if(blocks == NULL)
        return 0;


    for(uint32_t i = 0; i < blocks_needed; i++){

        blocks[i] = find_free_block(input,sb);

        if(blocks[i] == 0xFFFFFFFF) {

            // release blocks already allocated
            for(uint32_t j = 0; j < i; j++) {
                write_fat_entry(input, sb, blocks[j], 0);
            }

            free(blocks);
            return 0;
        }

        write_fat_entry(input, sb, blocks[i], 0xFFFFFFFF);
    }

    for(uint32_t i = 0; i < blocks_needed-1; i++){
        write_fat_entry(input, sb, blocks[i], blocks[i+1]);
    }

    write_fat_entry(input, sb, blocks[blocks_needed-1], 0xFFFFFFFF);

    *first_block = blocks[0];

    free(blocks);
    return 1;
}

void write_file_data(FILE *input, FILE *source, SuperBlock *sb, uint32_t first_block, uint32_t file_size){

    unsigned char buffer[sb->block_size];

    uint32_t remaining = file_size;
    uint32_t current = first_block;

    while(current != 0xFFFFFFFF) {

        memset(buffer,0,sb->block_size);
        uint32_t bytes = remaining < sb->block_size ? remaining : sb->block_size;

        fread(buffer,1,bytes,source);
        fseek(input, (uint64_t)current * sb->block_size, SEEK_SET);
        fwrite(buffer, sb->block_size, 1, input);

        remaining -= bytes;

        if(remaining == 0)
            break;

        current = get_fat_entry(input, sb,current);

    }
}

uint32_t get_fat_entry(FILE *input, SuperBlock *sb, uint32_t block_number) {

    uint32_t value;

    fseek(input, sb->fat_start * sb->block_size + block_number * 4, SEEK_SET);
    fread(&value,4,1,input);

    return ntohl(value);
}

void write_fat_entry(FILE *input,  SuperBlock *sb, uint32_t block, uint32_t value){

    uint32_t data = htonl(value);

    fseek(input, sb->fat_start * sb->block_size + block * 4, SEEK_SET);

    fwrite(&data,4,1,input);
} 

DirectoryEntry create_file_entry(char *filename, uint32_t first_block, uint32_t blocks, uint32_t file_size){

    DirectoryEntry entry;

    memset(&entry,0,sizeof(entry));

    entry.status = 0x03;

    entry.starting_block = first_block;
    entry.num_blocks = blocks;
    entry.file_size = file_size;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    uint16_t year = htons(t->tm_year + 1900);

    memcpy(entry.creation_time, &year, 2);

    entry.creation_time[2]=t->tm_mon+1;
    entry.creation_time[3]=t->tm_mday;
    entry.creation_time[4]=t->tm_hour;
    entry.creation_time[5]=t->tm_min;
    entry.creation_time[6]=t->tm_sec;

    memcpy(entry.modification_time, entry.creation_time, 7);

    strncpy(entry.filename, filename, 31);

    return entry;
}

int write_directory_entry(FILE *input, SuperBlock *sb, DirectoryEntry *directory, DirectoryEntry *new_file) {

    uint64_t offset = (uint64_t)directory->starting_block *  sb->block_size;

    int entries =  (directory->num_blocks * sb->block_size) / DIR_ENTRY_SIZE;

    unsigned char buffer[DIR_ENTRY_SIZE];

    for(int i=0;i<entries;i++) {


        fseek(input,  offset + i*DIR_ENTRY_SIZE, SEEK_SET);

        fread(buffer,  DIR_ENTRY_SIZE,  1, input);

        if(buffer[0] == 0) {
            unsigned char out[DIR_ENTRY_SIZE];

            memset(out,0,sizeof(out));

            out[0] = new_file->status;

            uint32_t temp;

            temp = htonl(new_file->starting_block);
            memcpy(out+1,&temp,4);

            temp = htonl(new_file->num_blocks);
            memcpy(out+5,&temp,4);

            temp = htonl(new_file->file_size);
            memcpy(out+9,&temp,4);

            memcpy(out+13, new_file->creation_time, 7);

            memcpy(out+20, new_file->modification_time,  7);

            memcpy(out+27, new_file->filename, 31);

            fseek(input, offset+i*DIR_ENTRY_SIZE, SEEK_SET);

            fwrite(out, DIR_ENTRY_SIZE, 1, input);

            return 1;
        }
    }

    return 0;
}

void free_file_blocks(FILE *input, SuperBlock *sb, uint32_t first_block) {

    uint32_t current = first_block;

    while(current != 0xFFFFFFFF) {

        uint32_t next = get_fat_entry(input, sb, current);

        // mark block free
        write_fat_entry(input, sb, current, 0);

        current = next;
    }
}

