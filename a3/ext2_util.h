#ifndef __EXT2_UTIL_H__
#define __EXT2_UTIL_H__

#include "ext2.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h> /* For memset */
#include <libgen.h> /* For basename and dirname*/
#include <getopt.h> /* For getting the required cmd line flags*/ //TODO: remove this
#include <string.h> /* useful tools*/

// =============================== Macros ==================================
#define NO 0
#define YES 1


// ======================= Frequently Used Variables =======================
 /* The input image_disk and its name*/
unsigned char *disk;
char *img_name;

/* Superblock*/
struct ext2_super_block *sb;
/* Group desciptor*/
struct ext2_group_desc *gd;
/* block bitmap blocks */
unsigned char *bb_ptr;
/*inode bitmap blocks */
unsigned char *ib_ptr;
/* Inode tables*/
struct ext2_inode *inode_tbl;

// ========================== Helper functions =============================

/* Initialize the superblock pointer sb, group descriptor gd, block and inode
bitmaps, and inode table.*/
void init_block_pointrs();

/* Directory entries need to be  aligned to 4-byte boundaries (For this
assignment mult is)*/
int round_up(unsigned num, unsigned mult);

/* Return the position of the very first bit equal to 0; otherwise return -1.
   Count is set to the total number of blocks or inodes*/
int get_free_bitmap(struct ext2_super_block *, struct ext2_group_desc *,
    unsigned char bitmap_ptr[], unsigned int count);


/* Return whether there is a directory entry in this block number block_num
  for the given token. If print_file is YES print the file (if found any).
        (A helper function for check_entries_for_path)
*/
int check_entries_for_path(unsigned block_num, char *token, int print_file,
    int *inode_ind, char *abs_path);


/* Get the inode index from the basename of the abs_path.
   If print_file set to YES or non-zero value then print the file if basename
   is a file o.w. do nothing (this is used for ext2_ls)*/
int get_inode_from_path(char *abs_path, int print_file);


#endif
