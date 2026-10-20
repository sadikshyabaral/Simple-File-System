#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "filesystem.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: could not open %s\n", argv[1]);
        return 1;
    }

    SuperBlock sb;

    if (!read_superblock(fp, &sb)) {
        fprintf(stderr, "Error: could not read superblock.\n");
        fclose(fp);
        return 1;
    }

    // print 
    printf("Super block information:\n");
    printf("Block size: %u\n", sb.block_size);
    printf("Block count: %u\n", sb.block_count);
    printf("FAT starts: %u\n", sb.fat_start);
    printf("FAT blocks: %u\n", sb.fat_blocks);
    printf("Root directory start: %u\n", sb.root_dir_start);
    printf("Root directory blocks: %u\n", sb.root_dir_blocks);

    //calculate how many FAT entries exist
    uint32_t fat_entries = (sb.fat_blocks * sb.block_size) / sizeof(uint32_t);


    // move pointer to FAT start 
   // fseek(fp, fat_start * block_size, SEEK_SET);

    if (fseek(fp, sb.fat_start * sb.block_size, SEEK_SET) != 0) {
        fprintf(stderr, "Error: could not seek to FAT.\n");
        fclose(fp);
        return 1;
    }


    // counters
    uint32_t free_blocks = 0;
    uint32_t reserved_blocks = 0;
    uint32_t allocated_blocks = 0;

    // read and process each FAT entry
    for (uint32_t i = 0; i < fat_entries; i++) {
        uint32_t entry;

        // read one FAT entry (4 bytes)
        if (fread(&entry, sizeof(uint32_t), 1, fp) != 1)
        {
            fprintf(stderr, "Error: could not read FAT.\n");
            fclose(fp);
            return 1;
        }

        // convert big endian to host byte order
        entry = ntohl(entry);

        // classify the entry
        if (entry == 0x00000000)
        {
            free_blocks++;
        }
        else if (entry == 0x00000001)
        {
            reserved_blocks++;
        }
        else
        {
            allocated_blocks++;
        }
    }

    printf("\nFAT information:\n");
    printf("Free Blocks: %u\n", free_blocks);
    printf("Reserved Blocks: %u\n", reserved_blocks);
    printf("Allocated Blocks: %u\n", allocated_blocks);



    fclose(fp);
    return 0;
}