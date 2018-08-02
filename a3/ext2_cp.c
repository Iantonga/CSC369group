#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h> /* Added this for strncpy*/
#include <getopt.h> /* For getting the required arguments*/
#include <libgen.h> /* For basename*/
#include <errno.h>
#include <sys/time.h> /* For timestamp */
#include <string.h> /* For memset */
#include "ext2.h"

unsigned char *disk;

void copy_file_test(FILE *fsrc, FILE *fdest) {
    char data;
	while (fread(&data, sizeof(char), 1, fsrc) == 1) {
		if (fwrite(&data, sizeof(char), 1, fdest) < 1) {
			perror("fwrite");
			exit(-1);
		}
	}
}
;
int get_free_bitmap(struct ext2_super_block *sb, struct ext2_group_desc *gd, unsigned char *bitmap_ptr) {

    int pos = 0;
    while (pos < sb->s_blocks_count) {
        int bit_map_byte = pos / 8;
        int bit_pos = pos % 8;
        if ((bitmap_ptr[bit_map_byte] >> bit_pos) & 1) {
            pos ++;
        } else {
            return pos;
        }
    }
    return -1;

}

int copy_file(FILE *fsrc, struct ext2_inode* inode, struct ext2_super_block *sb, struct ext2_group_desc *gd, unsigned char *blk_map_ptr) {
    char visited_blocks[sb->s_blocks_count]; 
    char data[1024];
    int total_bytes_read = 0;
    int block_bytes_read = 0;
    int index = 0;
    int pos_free = 0;
    unsigned char *block_ptr;
    size_t r;
    while ((r = fread(&data, sizeof(char), 1024, fsrc)) != 0) {
        pos_free = get_free_bitmap(sb, gd, blk_map_ptr);
        block_ptr = disk + EXT2_BLOCK_SIZE * pos_free;
        sprintf((char *) block_ptr, "%s", data);
        inode->i_block[index] = pos_free;
        block_bytes_read += r;
        total_bytes_read += r;
        index ++;
        if (index == 13) {
            // TODO
        } else if (index == 14) {
            // TODO
        } else {
            fprintf(stderr, "what?\n");
        }
    }

    return total_bytes_read;

}

void create_inode(int pos, struct ext2_inode *inode_tbl, unsigned char *ib_ptr, FILE *fsrc, struct ext2_super_block *sb, struct ext2_group_desc *gd, unsigned char *blk_map_ptr) {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    memset(&inode_tbl[pos], 0, sizeof(inode_tbl[pos]));
    inode_tbl[pos].i_mode |= 0x8000;
    inode_tbl[pos].i_ctime = tv.tv_usec;


    int size = copy_file(fsrc, &inode_tbl[pos], sb, gd, blk_map_ptr);
    //int size = copy_file(fsrc, &(inode_tbl[pos].i_block), sb, gd, blk_map_ptr);



}





void parse_cmd(int argc, char **argv, char **img, char **os_path, char **ext2_path) {
    char *usage = "Usage: ext2_cp <my_disk> <path_on_my_computer> <path_on_fs>\n";

    if (argc != 4) {
        fprintf(stderr, "%s", usage);
        exit(1);
    }

    *img = argv[1];
    *os_path = argv[2];
    *ext2_path = argv[3];

    /* Check whether the first character of abs_path start with root, if not
    print and error!*/
    if (*ext2_path[0] != '/') {
        fprintf(stderr, "ERROR: Absolute path should start with '/'\n");
        exit(1);
    }
}


int main(int argc, char **argv) {
    char *img_name;
    char *os_path;
    char *ext2_path;
    char *fsrc_name;
    FILE *fsrc;

    parse_cmd(argc, argv, &img_name, &os_path, &ext2_path);

    int fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
    	perror("mmap");
    	exit(1);
    }

    // Opening the source file that it'll be copied
    if((fsrc = fopen(os_path, "rb")) == NULL) {
        perror("fopen");
        exit(1);
    }

    /* The name of the source file (i.e. getting the basename of the path
    returns the file name) */
    if ((fsrc_name = basename(os_path)) == NULL) {
        perror("basename");
        exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    int bitmap_block_location = gd->bg_block_bitmap;
    unsigned char *bb_ptr = disk + EXT2_BLOCK_SIZE * bitmap_block_location;
    int bitmap_inode_location = gd->bg_inode_bitmap;
    unsigned char *ib_ptr = disk + EXT2_BLOCK_SIZE * bitmap_inode_location;
    unsigned int i_tbl_location = gd->bg_inode_table;
    struct ext2_inode *inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * i_tbl_location);

    // int free_bb_pos;
    // if (( free_bb_pos = get_free_bitmap(sb, gd, bb_ptr)) < 0) {
    //     return ENOSPC;
    // }
    int free_ib_pos;
    if (( free_ib_pos = get_free_bitmap(sb, gd, ib_ptr)) < 0) {
        return ENOSPC;
    }


    create_inode(free_ib_pos, inode_tbl, ib_ptr, fsrc, sb, gd, bb_ptr);

    /* If it doesn't exist make a new file in the ext2 */

    /*If the file already exists.*/





    return 0;
}
