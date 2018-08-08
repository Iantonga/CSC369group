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
#include "ext2_util.h"

#include <sys/stat.h>


unsigned char *disk;

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


int copy_file(FILE *fsrc, struct ext2_inode* inode, unsigned char blk_map_ptr[]) {
    char data[1024];
    memset(&data, 0, sizeof(data));
    int total_bytes_read = 0;
    int index = 0;
    int s_index = 0;
    int pos_free = 0;
    unsigned char *block_ptr;

    size_t r;
    while ((r = fread(&data, sizeof(char), 1024, fsrc)) != 0) {

        pos_free = get_free_bitmap(blk_map_ptr, sb->s_blocks_count);
        if (pos_free < 0) {
            return -ENOSPC;
        }

        gd->bg_free_blocks_count -= 1;
        sb->s_free_blocks_count -= 1;
        if (index < 12) {
            block_ptr = disk + EXT2_BLOCK_SIZE * (pos_free);
            sprintf((char *) block_ptr, "%s", data);
            inode->i_block[index] = pos_free;
            index ++;

        } else if (index == 12) {
            if (s_index == 0) {
                inode->i_block[index] = pos_free;
                pos_free = get_free_bitmap(blk_map_ptr, sb->s_blocks_count);
                if (pos_free < 0) {
                    fprintf(stderr, "Not enough space!\n");
                    exit(ENOSPC);
                }
                gd->bg_free_blocks_count -= 1;
                sb->s_free_blocks_count -= 1;
            }
            if ((int)sb->s_free_blocks_count >= 0) {
                block_ptr = disk + EXT2_BLOCK_SIZE * pos_free;
                sprintf((char *) block_ptr, "%s", data);
                unsigned int *s_indir = (unsigned int *)(disk + inode->i_block[index] * EXT2_BLOCK_SIZE);
                s_indir[s_index] = pos_free;
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

        total_bytes_read += r;

        memset(&data, 0, sizeof(data));

    }

    return total_bytes_read;

}

int create_inode(int pos, struct ext2_inode* inode, FILE *fsrc, unsigned char blk_map_ptr[]) {
    memset(&inode_tbl[pos], 0, sizeof(inode_tbl[pos]));
    inode[pos].i_mode |= EXT2_S_IFREG;
    inode[pos].i_atime = (unsigned int)time(NULL);
    inode[pos].i_ctime = (unsigned int)time(NULL);
    inode[pos].i_mtime = (unsigned int)time(NULL);
    int size = copy_file(fsrc, &inode[pos], blk_map_ptr);
    if (size < 0) {
        return -ENOSPC;
    }

    inode[pos].i_size = size;
    inode[pos].i_blocks = my_ceil(size / (EXT2_BLOCK_SIZE / 2.0));
    inode[pos].i_links_count = 1;

    // WARNING: This should be after directory entry!!!
    gd->bg_free_inodes_count -= 1;
    sb->s_free_inodes_count -= 1;


    return 1;

}


/* Parse the command line and set them to them appropriate values*/
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
    img_name = NULL;
    char *os_path;
    char *ext2_path;
    FILE *fsrc;

    struct stat buf;


    parse_cmd(argc, argv, &img_name, &os_path, &ext2_path);

    int fd = open(img_name, O_RDWR);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
    	perror("mmap");
    	exit(1);
    }

    // WARNING: Also use lstat to check if the fsrc is a file or a directory!!!
    // Opening the source file that it'll be copied
    if((fsrc = fopen(os_path, "rb")) == NULL) {
        perror("fopen");
        exit(1);
    }

    if ((lstat(os_path, &buf)) == -1) {
        perror("lstat");
        exit(1);
    }
    if(!(S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode))) {
        fprintf(stderr, "Cannot be a directory\n");
    }


    /* The name of the source file (i.e. getting the basename of the path
    returns the file name) */

    char *tmp;
    if ((tmp = basename(os_path)) == NULL) {
        perror("basename");
        exit(1);
    }

    char fsrc_name[strlen(tmp) + 1];
    fsrc_name[strlen(tmp)] = '\0';
    strncpy(fsrc_name, tmp, strlen(tmp));


    init_block_pointrs();

    char *parent_path;
    int inode_index = get_inode_from_path(ext2_path, NO);
    if (inode_index < 0) {
        if ((parent_path = dirname(ext2_path)) == NULL) {
            perror("dirname");
            exit(1);
        }
        if (get_inode_from_path(parent_path, NO) >= 0) {
            char *name;
            if ((name = basename(ext2_path)) == NULL) {
                perror("dirname");
                exit(1);
            }
            fsrc_name = name;
        } else {
            fprintf(stderr, "No such file or directory\n");
            return ENOENT;
        }

    }
    printf("%s\n", fsrc_name);

    // TODO: if file name is given just output a new file!

    /* NOTE: Copy of the block bitmap and inode bitmap*/
    unsigned char block_bitmap_copy[sb->s_blocks_count / 8];
    for (int i = 0; i < sb->s_blocks_count / 8; i++) {
        block_bitmap_copy[i] = bb_ptr[i];
    }

    unsigned char inode_bitmap_copy[sb->s_inodes_count / 8];
    for (int i = 0; i < sb->s_inodes_count / 8; i++) {
        inode_bitmap_copy[i] = ib_ptr[i];
    }

    int isfound = 0;
    if (inode_tbl[inode_index].i_mode & EXT2_S_IFDIR) {

        int free_ib_pos;
        if (( free_ib_pos = get_free_bitmap(ib_ptr, sb->s_inodes_count)) < 0) {
            return ENOSPC;
        }

        int err = create_inode(free_ib_pos, inode_tbl, fsrc, block_bitmap_copy);
        if (err < 0) {
            fprintf(stderr, "Not enough space!\n");
            return ENOSPC;
        }

        // Now we insert it into the directory entry!
        for (int j = 0; j < 15 && !isfound; j++) {
            if (j < 12) {
                struct ext2_dir_entry_2 *curr_de = (struct ext2_dir_entry_2 *)(disk +
                    inode_tbl[inode_index].i_block[j] * EXT2_BLOCK_SIZE);
                struct ext2_dir_entry_2 *prev_de = NULL;

                if (inode_tbl[inode_index].i_block[j]) {
                    int curr_entry = 0;
                    int newf_metadata_size = sizeof(unsigned int) + sizeof(short) +
                        sizeof(char) * 2 + strlen(fsrc_name);

                    while (curr_entry < EXT2_BLOCK_SIZE) {
                        int metadata_size = sizeof(unsigned int) +
                            sizeof(short) + sizeof(char) * 2 + curr_de->name_len;
                        metadata_size = round_up(metadata_size, 4);
                        int remaining_bytes = curr_de->rec_len - (metadata_size);
                        if ( remaining_bytes >= newf_metadata_size) {
                            if (inode_tbl[curr_de->inode].i_dtime) {
                                if (newf_metadata_size <= metadata_size) {
                                    curr_de->inode = free_ib_pos + 1;
                                    if (prev_de != NULL) {
                                        curr_de->rec_len = prev_de->rec_len;
                                    }
                                    curr_de->name_len = strlen(fsrc_name);
                                    curr_de->file_type = EXT2_FT_REG_FILE;
                                    strncpy(curr_de->name, fsrc_name, strlen(fsrc_name));
                                    if (prev_de != NULL) {
                                        prev_de->rec_len = metadata_size;
                                    }
                                }
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
                    if (( free_bb_pos = get_free_bitmap( bb_ptr, sb->s_blocks_count)) < 0) {
                        fprintf(stderr, "not enough space!\n");
                        return ENOSPC;
                    }
                    inode_tbl[inode_index].i_block[j] = free_bb_pos + 1;

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

        for (int i = 0; i < 15; i++) {
            int bit_map_byte =  inode_tbl[inode_index].i_block[i] / 8;
            int bit_pos = inode_tbl[inode_index].i_block[i] % 8;
            bb_ptr[bit_map_byte] =  (bb_ptr[bit_map_byte] >> bit_pos) & 0;
            inode_tbl[inode_index].i_block[i] = 0;
        }


        int size = copy_file(fsrc, &inode_tbl[inode_index], block_bitmap_copy);
        if (size < 0) {
            return ENOSPC;
        }
        inode_tbl[inode_index].i_size = size;

    }

    // Update the bb_ptr and ib_ptr;
    for (int i = 0; i < (sb->s_blocks_count / 8); i ++) {
        bb_ptr[i] = block_bitmap_copy[i];
    }

    for (int i = 0; i < (sb->s_inodes_count / 8); i ++) {
        ib_ptr[i] = inode_bitmap_copy[i];
    }

    return 0;
}
