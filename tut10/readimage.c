#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

unsigned char *disk;


int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: readimg <image file name>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
    	perror("mmap");
    	exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);

    /* Task 1*/
    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    printf("Block group:\n");
    printf("    block bitmap: %d\n", gd->bg_block_bitmap);
    printf("    inode bitmap: %d\n", gd->bg_inode_bitmap);
    printf("    inode table: %d\n", gd->bg_inode_table);
    printf("    free blocks: %d\n", gd->bg_free_blocks_count);
    printf("    free inodes: %d\n", gd->bg_free_inodes_count);
    printf("    used_dirs: %d\n", gd->bg_used_dirs_count);


    /* Task 2.1: bitmap blocks*/
    printf("Block bitmap: ");
    // The group descriptor often knows that blocks bitmap located at 3.
    int bitmap_block_location = gd->bg_block_bitmap;

    // Pointer to the beginning of the block bitmap (note: this is little
    // endian;e.g. 0x80003fff is stored in mempry like ff3f0008).
    unsigned char *bb_ptr = disk + EXT2_BLOCK_SIZE * bitmap_block_location;

    // Since every bit in block bitmap corresponds to a data block, we need
    // to get total number of blocks and divide that by 8 (1 byte) because
    // we print space between each byte.
    int total_blocks = sb->s_blocks_count;
    for (int i = 0; i < total_blocks / 8; i++) {
        for (int j = 0; j < 8; j++) {
            printf("%d", (*(bb_ptr + i) >> j) & 1);
        }
        printf(" ");
    }

    printf("\nInode bitmap: ");
    int bitmap_inode_location = gd->bg_inode_bitmap;

    unsigned char *bi_ptr = disk + EXT2_BLOCK_SIZE * bitmap_inode_location;

    int total_inodes = sb->s_inodes_count;
    for (int i = 0; i < total_inodes / 8; i++) {
        for (int j = 0; j < 8; j++) {
            printf("%d", (*(bi_ptr + i) >> j) & 1);
        }
        printf(" ");
    }

    /* Task 2.2: inode table*/
    // From ext2.h Inodes 1 to 10 are special inode numbers and first
    // non-reserved inode for old ext2 filesystem is inode #11.
    // printf("\n\nfirst nonres inode: %d, block group #: %d", sb->s_first_ino, sb->s_block_group_nr);
    printf("\n\nInodes:\n");

    unsigned int i_tbl_location = gd->bg_inode_table;
    struct ext2_inode *it = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * i_tbl_location);

    for (int i = 0; i < total_inodes; i++) {
        // If the inode is either root or none-reserved (do not print info
        // about inode 11 at index 10 since it is the lost+found folder) and print
        // those inodes that have size > 0!
        char type = ' ';
        int inode_num = i + 1;
        if ((i == 1 || inode_num > EXT2_GOOD_OLD_FIRST_INO) && it[i].i_size > 0) {
            if (it[i].i_mode & EXT2_S_IFREG) {
                type = 'f';
            } else if (it[i].i_mode & EXT2_S_IFDIR) {
                type = 'd';
            } else if (it[i].i_mode & EXT2_S_IFLNK) {
                type = 'l';
            }

            printf("[%d] type: %c size: %d links: %d blocks: %d\n",
                inode_num, type, it[i].i_size, it[i].i_links_count, it[i].i_blocks);


            printf("[%d] Blocks: ", inode_num);

            for (int ptr_i = 0; ptr_i < 15; ptr_i++) {
                if (it[i].i_block[ptr_i]) {
                    if (ptr_i < 12) {
                        printf(" %d", it[i].i_block[ptr_i]);


                        // single indirect pointer
                    } else if (ptr_i == 12 ) {
                        printf("\n\t");
                        printf("  Single indirect blocks: ");
                        unsigned int *s_indir = (unsigned int *)(disk + it[i].i_block[ptr_i] * EXT2_BLOCK_SIZE);
                        for (int s = 0; s < EXT2_BLOCK_SIZE / sizeof(unsigned int); s++) {
                            if (s_indir[s]) {
                                printf(" %d", s_indir[s]);
                            }
                        }

                        // double indirect poitner
                    } else if (ptr_i == 13) {
                        printf("\n\t");
                        printf("\n    Duble indirect blocks: ");
                        unsigned int *d_indir = (unsigned int *)(disk + it[i].i_block[ptr_i] * EXT2_BLOCK_SIZE);
                        for (int d = 0; d < EXT2_BLOCK_SIZE / sizeof(unsigned int); d++) {
                            if (d_indir[d]) {
                                unsigned int *s_indir = (unsigned int *)(disk + d_indir[d] * EXT2_BLOCK_SIZE);
                                for (int s = 0; s < EXT2_BLOCK_SIZE / sizeof(unsigned int); s++) {
                                    printf(" %d", s_indir[s]);
                                }
                            }
                        }
                        // triple indirect pointer
                    } else {
                        printf("\n\t");
                        printf("\n    Triple indirect blocks: ");
                        unsigned int *t_indir = (unsigned int *)(disk + it[i].i_block[ptr_i] * EXT2_BLOCK_SIZE);
                        for (int t = 0; t < EXT2_BLOCK_SIZE / sizeof(unsigned int); t++) {
                            unsigned int *d_indir = (unsigned int *)(disk + t_indir[t] * EXT2_BLOCK_SIZE);
                            for (int d = 0; d < EXT2_BLOCK_SIZE / sizeof(unsigned int); d++) {
                                if (d_indir[d]) {
                                    unsigned int *s_indir = (unsigned int *)(disk + d_indir[d] * EXT2_BLOCK_SIZE);
                                    for (int s = 0; s < EXT2_BLOCK_SIZE / sizeof(unsigned int); s++) {
                                        printf(" %d", s_indir[s]);
                                    }
                                }
                            }
                        }

                    }
                }
            }
            printf("\n");
        }
    }

    /* Task 3: Directories*/
    printf("\nDirectory blocks:\n");
    for (int i = 0; i < total_inodes; i ++) {
        int inode_num = i + 1;

        // Care about root and nonreserved and also
        // this time we consider those directories that are empty.
        if (i == 1 || inode_num >= EXT2_GOOD_OLD_FIRST_INO) {
            if (it[i].i_mode & EXT2_S_IFDIR) {
				
				for (int j = 0; j < 15; j++){
					if (it[i].i_block[j]) {
						if (j < 12) {	
							
							printf("   DIR BLOCK NUM: %d (for inode %d)\n", it[i].i_block[j], inode_num);
							struct ext2_dir_entry_2 *de = (struct ext2_dir_entry_2 *)(disk + it[i].i_block[j] * EXT2_BLOCK_SIZE);
							int entry = 0;
							while (entry < EXT2_BLOCK_SIZE / sizeof(struct ext2_dir_entry_2)) {
								printf("Inode: %d rec_len: %d name_len: %c type= %c name=%s\n", de->inode, de->rec_len, de->name_len,
									de->file_type, de->name);
								entry += de->rec_len;
							}
							
						}

					}
					
				}
            }
        }
    }


    return 0;

}
