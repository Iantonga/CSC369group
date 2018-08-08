#include "ext2_util.h"


void init_block_pointrs() {
    sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    int bitmap_block_location = gd->bg_block_bitmap;
    bb_ptr = disk + EXT2_BLOCK_SIZE * bitmap_block_location;
    int bitmap_inode_location = gd->bg_inode_bitmap;
    ib_ptr = disk + EXT2_BLOCK_SIZE * bitmap_inode_location;
    unsigned int i_tbl_location = gd->bg_inode_table;
    inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * i_tbl_location);
}


int round_up(unsigned num, unsigned mult) {
    int r =  num % mult;
    if (!r) {
        return num;
    }
    return (num + mult) - r;
}


int my_ceil(float num) {
    int inum = (int)num;
    if (num == (float)inum) {
        return inum;
    }
    return inum + 1;
}


int get_free_bitmap(unsigned char bitmap_ptr[], unsigned int count) {

    int pos = 0;
    while (pos < count) {
        int bit_map_byte = pos / 8;
        int bit_pos = pos % 8;
        if ((bitmap_ptr[bit_map_byte] >> bit_pos) & 1) {
            pos ++;
        } else {
            // Flip the bit in the copy of the bitmap_ptr.
            char found_bit = 1 << bit_pos;
            bitmap_ptr[bit_map_byte] |= found_bit;
            return pos;
        }
    }

    // Did not find a free space.
    return -1;

}

void set_free_bitmap(int i, unsigned char bitmap_ptr[]) {
    int bit_map_byte = i / 8;
    int bit_pos = i % 8;
    char tmp = ~(1 << (bit_pos));
    bitmap_ptr[bit_map_byte] &= tmp;
    // int bit_map_byte = i / 8;
    // int bit_pos = i % 8;
    // bitmap_ptr[bit_map_byte] = (bitmap_ptr[bit_map_byte] >> bit_pos) & 0;
}


/* Return whether there is a directory entry in this block number block_num
  for the given token. If print_file is YES print the file (if found any).

  Return 1 if found such entry if not 0 is returned; Also,
  return -1 if a file is treated as a directory*/
int check_entries_for_path(unsigned block_num, char *token, int print_file,
    int *inode_ind, char *abs_path) {

    struct ext2_dir_entry_2 *de = (struct ext2_dir_entry_2 *)(disk +
        block_num * EXT2_BLOCK_SIZE);

    int isfound = 0;
    int curr_entry = 0;
    while (curr_entry < EXT2_BLOCK_SIZE) {
        int size = (int) de->name_len;
        char tmp_name[size + 1];
        tmp_name[size] = '\0';
        strncpy(tmp_name, de->name, size);
        if (!strncmp(token, tmp_name, strlen(token)) &&
                strlen(token) == size) {
            isfound = 1;
            *inode_ind = de->inode - 1;

            /*A file name in the entry should not end with
            an '/'*/
            if (de->file_type != EXT2_FT_DIR) {
                char *substr = strstr(abs_path, token);
                int pos = substr - abs_path;
                if (abs_path[pos + size] == '/') {
                    return -1;
                }

                /*This is for ext2_ls to just print the
                file name if the path leads to a file*/
                if (print_file) {
                    printf("%s\n", tmp_name);
                }
            }
            break;
        }
        curr_entry += de->rec_len;
        de = (void *)de + de->rec_len;
    }
    return isfound;
}


/* Return the index of the inode of the basename of the absolute path abs_path.
   The sec parameter print_file is a non-zero (ideally set to YES) then print
   the file if the basename is a a file. Otherwise return a negative number as
   an error.

    For Errors:
        1. Return -1 if there is a file in the path and it is treated as a
        directory (e.g. /dir1/afile/dir2)
        2. Return -2 if there is no entry exists corresponding to the path.
*/
int get_inode_from_path(char *abs_path, int print_file) {
    /* NOTE: for 'strtok' need to copy abs_path to an array since there is a bug
    if we use const it gets rid of the last '/'*/
    char path[strlen(abs_path) + 1];
    path[strlen(abs_path)] = '\0';
    strncpy(path, abs_path, strlen(abs_path));

    /* We assume that path start at '/'. we subtract 1 since inode starts
    at 1 instead of 0. */
    int inode_ind = EXT2_ROOT_INO - 1;

    char *token = NULL;
    token = strtok(path, "/");
    while( token != NULL ) {

        // Whether the file in the path is in the directory entry
        int isfound = 0;
        if (inode_tbl[inode_ind].i_mode & EXT2_S_IFDIR) {
            for (int b = 0; b < 15 && !isfound; b++) {
                if (b < 12) {
                    if (inode_tbl[inode_ind].i_block[b]) {
                        isfound = check_entries_for_path(inode_tbl[inode_ind].i_block[b],
                            token, print_file, &inode_ind, abs_path);
                        if (isfound == -1) {
                            return -1;
                        }

                    }
                } else if (b == 12) { // Single indirect pointer: i_block[12]
                    // TODO: delete the print statement here
                    // printf("checking entries in single indirect\n");
                    unsigned int *s_indir = (unsigned int *)(disk +
                        inode_tbl[inode_ind].i_block[b] * EXT2_BLOCK_SIZE);
                        for (int s = 0; s < (EXT2_BLOCK_SIZE / sizeof(unsigned)) && !isfound; s++) {
                            if (s_indir[s]) {
                                isfound = check_entries_for_path(s_indir[s],
                                    token, print_file, &inode_ind, abs_path);
                                if (isfound == -1) {
                                    return -1;
                                }
                            }
                        }
                        if (isfound == 0) {
                            return -2;
                        }

                } else {
                    fprintf(stderr, "i_block[13] & i_block[14] not \
                        need to be implemented\n");
                    exit(1);
                }
            }
            if (isfound == 0) {
                return -2;
            }

        } else {
            return -2;
        }
        // Next token
        token = strtok(NULL, "/");
   }

   return inode_ind;
}


/**/
char *concat_to_path(char *link_path, char *fname_src) {
    int size = strlen(link_path) + 1 + strlen(fname_src) + 1;
    char *result = (void *)malloc(size * sizeof(char));
    sprintf(result, "%s/%s", link_path, fname_src);
    result[size] = '\0';
    return result;
}
