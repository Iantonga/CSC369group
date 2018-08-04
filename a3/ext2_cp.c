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
#include <assert.h>
#include <errno.h>
#include <time.h> /* For timestamp */
#include <string.h> /* For memset */
#include "ext2.h"

unsigned char *disk;


int my_ceil(float num) {
    int inum = (int)num;
    if (num == (float)inum) {
        return inum;
    }
    return inum + 1;
}

int round_up(int num, int mult) {
    int r =  num % mult;
    if (!r) {
        return num;
    }
    return (num + mult) - r;
}

// WARNING: This is a reptitive code!!!!!!!!!
int get_inode_from_path(char *abs_path, int print_file) {
    /* NOTE(strtok): need to copy abs_path to an array since they said there is a bug
    if we use const it gets rid of the last '/'*/
    char path[strlen(abs_path) + 1];
    path[strlen(abs_path)] = '\0';
    strncpy(path, abs_path, strlen(abs_path));


    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    unsigned int i_tbl_location = gd->bg_inode_table;
    struct ext2_inode *inodes = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * i_tbl_location);


    /* We assume that path start at '/'. we subtract 1 since inode starts
    at 1 instead of 0. */
    int inode_ind = EXT2_ROOT_INO - 1;

    char *token = NULL;
    token = strtok(path, "/");
    while( token != NULL ) {
        // printf( "token: %s\n", token);
        int isfound = 0;
        if (inodes[inode_ind].i_mode & EXT2_S_IFDIR) {
            for (int b = 0; b < 15 && !isfound; b++) {
                if (b < 12) {
                    if (inodes[inode_ind].i_block[b]) {
                        struct ext2_dir_entry_2 *de = (struct ext2_dir_entry_2 *)(disk + inodes[inode_ind].i_block[b] * EXT2_BLOCK_SIZE);
                        int curr_entry = 0;

                        while (curr_entry < EXT2_BLOCK_SIZE) {
                            int size = (int) de->name_len;
                            char tmp_name[size + 1];
                            tmp_name[size] = '\0';
                            strncpy(tmp_name, de->name, size);
                            // printf("%d %s\n", de->inode, tmp_name);
                            if (!strncmp(token, tmp_name, strlen(token)) && strlen(token) == size) {
                                isfound = 1;
                                inode_ind = de->inode - 1;
                                if (de->file_type != EXT2_FT_DIR) {
                                    char *substr = strstr(abs_path, token);
                                    int pos = substr - abs_path;
                                    if (abs_path[pos + size] == '/') {
                                        fprintf(stderr, "No such file or directory\n");
                                        return -ENOENT;
                                    }
                                    if (print_file) {
                                        printf("%s\n", tmp_name);
                                    }
                                }
                                break;
                            }
                            curr_entry += de->rec_len;
                            de = (void *)de + de->rec_len;
                        }
                        if (!isfound) {
                            fprintf(stderr, "No such file or directory\n");
                            return -ENOENT;
                        }
                    }
                } else if (b == 12) {
                    // TODO: Implement this
                } else {
                    // TODO: Implement this
                }
            }

        } else {
            fprintf(stderr, "No such file or directory\n");
            return ENOENT;

        }

        token = strtok(NULL, "/");
   }

   return inode_ind;
}

// TODO: need to update the gd and sb as well.

int get_free_bitmap(struct ext2_super_block *sb, struct ext2_group_desc *gd, unsigned char bitmap_ptr[], unsigned int count) {

    // int pos = 0;
    // while (pos < sb->s_blocks_count) {
    //     int bit_map_byte = pos / 8;
    //     int bit_pos = pos % 8;
    //     if ((bitmap_ptr[bit_map_byte] >> bit_pos) & 1) {
    //         pos ++;
    //     } else {
    //         return pos;
    //     }
    // }
    // return -1;
    int bit_map_byte = 34 / 8;
    int bit_pos = 34 % 8;
    char found_bit = 1 << bit_pos;
    bitmap_ptr[bit_map_byte] |= found_bit;
    return 34;

}

/* Having a copy of the bitmap is better to work with*/
int get_free_bitmap2(struct ext2_super_block *sb, struct ext2_group_desc *gd, unsigned char bitmap_ptr[], unsigned int count) {

    int pos = 0;
    while (pos < count) {
        int bit_map_byte = pos / 8;
        int bit_pos = pos % 8;
        if ((bitmap_ptr[bit_map_byte] >> bit_pos) & 1) {
            pos ++;
        } else {
            char found_bit = 1 << bit_pos;
            bitmap_ptr[bit_map_byte] |= found_bit;
            return pos;
        }
    }
    return -1;

}

int copy_file(FILE *fsrc, struct ext2_inode* inode, struct ext2_super_block *sb, struct ext2_group_desc *gd, unsigned char blk_map_ptr[]) {
    char data[1024];
    memset(&data, 0, sizeof(data));
    int total_bytes_read = 0;
    int block_bytes_read = 0;
    int index = 0;
    int s_index = 0;
    // int d_index = 0; /* Not needed for this assignement*/
    // int t_index = 0;  /* Not needed fo this assignment*/
    int pos_free = 0;
    unsigned char *block_ptr;

    size_t r;
    while ((r = fread(&data, sizeof(char), 1024, fsrc)) != 0) {
        pos_free = get_free_bitmap2(sb, gd, blk_map_ptr, sb->s_blocks_count);
        if (pos_free < 0) {
            return -ENOSPC;
        }
        gd->bg_free_blocks_count -= 1;
        sb->s_free_blocks_count -= 1;
        if (index < 12) {
            block_ptr = disk + EXT2_BLOCK_SIZE * (pos_free + 1);
            sprintf((char *) block_ptr, "%s", data);
            inode->i_block[index] = pos_free + 1;
            index ++;

        } else if (index == 12) {
            if (s_index == 0) {
                inode->i_block[index] = pos_free + 1;
                pos_free = get_free_bitmap2(sb, gd, blk_map_ptr, sb->s_blocks_count);
                if (pos_free < 0) {
                    return -ENOSPC;
                }
                gd->bg_free_blocks_count -= 1;
                sb->s_free_blocks_count -= 1;
            }
            if (s_index < EXT2_BLOCK_SIZE / sizeof(unsigned int)) {

                block_ptr = disk + EXT2_BLOCK_SIZE * (pos_free + 1);
                sprintf((char *) block_ptr, "%s", data);
                unsigned int *s_indir = (unsigned int *)(disk + inode->i_block[index] * EXT2_BLOCK_SIZE);
                s_indir[s_index] = pos_free + 1;
                s_index ++;
            } else {
                s_index = 0;
                index ++;
            }

        } else if (index == 13) {
            fprintf(stderr, "i_block[13] is not required to  be implemented\n" );
            exit(1);
        } else if (index == 14) {
            fprintf(stderr, "i_block[14] is not required to  be implemented\n" );
            exit(1);
        } else {
            fprintf(stderr, "Error index out of bound for i_block[15]\n");
            exit(1);
        }

        block_bytes_read += r;
        total_bytes_read += r;

        memset(&data, 0, sizeof(data));

    }

    return total_bytes_read;

}

int create_inode(int pos, struct ext2_inode *inode_tbl, unsigned char *ib_ptr, FILE *fsrc, struct ext2_super_block *sb, struct ext2_group_desc *gd, unsigned char blk_map_ptr[]) {
    memset(&inode_tbl[pos], 0, sizeof(inode_tbl[pos])); // NOTE: problem may arise!
    inode_tbl[pos].i_mode |= 0x8000;
    // inode_tbl[pos].i_mode |= 0x002;
    // inode_tbl[pos].i_mode |= 0x004;
    inode_tbl[pos].i_atime = (unsigned int)time(NULL);
    inode_tbl[pos].i_ctime = (unsigned int)time(NULL);
    inode_tbl[pos].i_mtime = (unsigned int)time(NULL);

    int size = copy_file(fsrc, &inode_tbl[pos], sb, gd, blk_map_ptr);
    if (size < 0) {
        return -ENOSPC;
    }

    inode_tbl[pos].i_size = size;
    inode_tbl[pos].i_blocks = my_ceil(size / (EXT2_BLOCK_SIZE / 2.0));
    inode_tbl[pos].i_links_count = 1;

    gd->bg_free_inodes_count -= 1;
    sb->s_free_inodes_count -= 1;


    return 1;

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


void dude(unsigned char b[], int size) {
    int i;
    int k;
    // printf("%d\n", num_bit);
    printf("bitmap: ");
    /* Looping through the bit map block byte by byte. */
    for (i = 0; i < size; i++) {

        /* Looping through each bit a byte. */
        for (k = 0; k < 8; k++) {
            printf("%d", (b[i] >> k) & 1);
        }
        printf(" ");
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


    // Get the inode index of the path specified!
    int inode_index = get_inode_from_path(ext2_path, 0);
    if (inode_index < 0) {
        return ENOENT;
    }

    /* NOTE: Copy of the block bitmap*/
    unsigned char block_bitmap_copy[sb->s_blocks_count / 8];
    for (int i = 0; i < 16; i++) {
        block_bitmap_copy[i] = bb_ptr[i];
    }

    // WARNING: repetitive code
    int isfound = 0;
    if (inode_tbl[inode_index].i_mode & EXT2_S_IFDIR) {

        // ==================================NEW===========================

        int free_ib_pos;
        if (( free_ib_pos = get_free_bitmap2(sb, gd, ib_ptr, sb->s_inodes_count)) < 0) {
            return ENOSPC;
        }

        int err = create_inode(free_ib_pos, inode_tbl, ib_ptr, fsrc, sb, gd, block_bitmap_copy);
        if (err < 0) {
            fprintf(stderr, "Not enough space!\n");
            return ENOSPC;
        }
        // ================================================================
        for (int j = 0; j < 15 && !isfound; j++) {
            if (j < 12) {
                struct ext2_dir_entry_2 *curr_de = (struct ext2_dir_entry_2 *)(disk + inode_tbl[inode_index].i_block[j] * EXT2_BLOCK_SIZE);
                struct ext2_dir_entry_2 *prev_de = NULL;
                if (inode_tbl[inode_index].i_block[j]) {
                    int curr_entry = 0;
                    int newf_metadata_size = sizeof(unsigned int) + sizeof(short) + sizeof(char) * 2 + strlen(fsrc_name);
                    while (curr_entry < EXT2_BLOCK_SIZE) {
                        int metadata_size = sizeof(unsigned int) + sizeof(short) + sizeof(char) * 2 + curr_de->name_len;
                        metadata_size = round_up(metadata_size, 4);
                        int remaining_bytes = curr_de->rec_len - (metadata_size);
                        if ( remaining_bytes >= newf_metadata_size) {
                            // If it is marked as deleted
                            // WARNING: repetitive
                            if (inode_tbl[curr_de->inode].i_dtime) {
                                prev_de = curr_de;
                                curr_de->inode = free_ib_pos + 1;
                                curr_de->rec_len = prev_de->rec_len - metadata_size;
                                curr_de->name_len = strlen(fsrc_name);
                                curr_de->file_type = EXT2_FT_REG_FILE;
                                prev_de->rec_len = metadata_size;

                            } else {
                                prev_de = curr_de;
                                curr_de = (void *)curr_de + metadata_size;
                                curr_de->inode = free_ib_pos + 1;
                                curr_de->rec_len = prev_de->rec_len - metadata_size;
                                curr_de->name_len = strlen(fsrc_name);
                                curr_de->file_type = EXT2_FT_REG_FILE;
                                strncpy(curr_de->name, fsrc_name, strlen(fsrc_name));
                                prev_de->rec_len = metadata_size;


                            }
                            isfound = 1;
                            break;

                        } else {
                            prev_de = curr_de;
                            curr_entry += curr_de->rec_len;
                            curr_de = (void *)curr_de + curr_de->rec_len;
                        }
                    }
                } else {
                    int free_bb_pos;
                    if (( free_bb_pos = get_free_bitmap2(sb, gd, bb_ptr, sb->s_blocks_count)) < 0) {
                        return ENOSPC;
                    }
                    inode_tbl[inode_index].i_block[j] = free_bb_pos;

                    curr_de->inode = free_ib_pos + 1;
                    curr_de->rec_len = EXT2_BLOCK_SIZE;
                    curr_de->name_len = strlen(fsrc_name);
                    curr_de->file_type = EXT2_FT_REG_FILE;
                    strncpy(curr_de->name, fsrc_name, strlen(fsrc_name));
                }
            } else {
                // TODO: Implement the other one for j == 12 only
            }
        }
    } else if (inode_tbl[inode_index].i_mode & EXT2_S_IFREG){
        // we overwrite

        // Set the block of the file that is being overwritten to 0.
        for (int i = 0; i < 15; i++) {
            int bit_map_byte =  inode_tbl[inode_index].i_block[i] / 8;
            int bit_pos = inode_tbl[inode_index].i_block[i] % 8;
            bb_ptr[bit_map_byte] =  (bb_ptr[bit_map_byte] >> bit_pos) & 0;
            inode_tbl[inode_index].i_block[i] = 0;
        }


        int size = copy_file(fsrc, &inode_tbl[inode_index], sb, gd, block_bitmap_copy);
        if (size < 0) {
            return -ENOSPC;
        }
        inode_tbl[inode_index].i_size = size;

    }

    // Update the bb_ptr;
    for (int i = 0; i < 16; i ++) {
        bb_ptr[i] = block_bitmap_copy[i];
    }







    return 0;
}
