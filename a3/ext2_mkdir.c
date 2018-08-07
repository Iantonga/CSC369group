
#include <assert.h>
#include <errno.h>
#include <time.h> /* For timestamp */
#include "ext2_util.h"

/*
Return -1 to indicate EEXIST
Return -2 to indicate ENOENT
*/
int get_parent_ino(char *path, char **dir_name) {
    if ((*dir_name = basename(path)) == NULL) {
        perror("basename");
        exit(1);
    }

    int status, index;
    if ((status = get_inode_from_path(path, NO)) >= 0) {
        return -1;
    } else if (status == -1) {
        return -2;
    }

    char *parent_path;
    if ((parent_path = dirname(path)) == NULL) {
        perror("dirname");
        exit(1);
    }

    if ((index = get_inode_from_path(parent_path, NO)) < 0) {
        return -2;
    }

    return index;
}


int create_inode_dir(struct ext2_inode *inode_tbl, unsigned char *ib_ptr, struct ext2_super_block *sb, struct ext2_group_desc *gd, unsigned char blk_map_ptr[]) {
    int pos;
    if (( pos = get_free_bitmap(sb, gd, ib_ptr, sb->s_inodes_count)) < 0) {
        return -1;
    }
    sb->s_free_inodes_count --;
    gd->bg_free_inodes_count -- ;

    memset(&inode_tbl[pos], 0, sizeof(inode_tbl[pos]));
    inode_tbl[pos].i_mode |= EXT2_S_IFDIR;
    // inode_tbl[pos].i_mode |= 0x002;
    // inode_tbl[pos].i_mode |= 0x004;
    inode_tbl[pos].i_atime = (unsigned int)time(NULL);
    inode_tbl[pos].i_ctime = (unsigned int)time(NULL);
    inode_tbl[pos].i_mtime = (unsigned int)time(NULL);
    inode_tbl[pos].i_size = EXT2_BLOCK_SIZE;
    inode_tbl[pos].i_blocks = 2;
    inode_tbl[pos].i_links_count = 2;


    /*1. update _dirs
        1.1) used_dirs: N
        2.2) free inodes: N
        2.3) free blocks: N
        2.4) links (for root): N*/

    // WARNING: sb and gd should be updated after
    gd->bg_used_dirs_count ++;


    return pos;
}


int create_dir(struct ext2_inode *inode_tbl, unsigned char *ib_ptr, unsigned char *bb_ptr, int par_i, int new_i, struct ext2_super_block *sb, struct ext2_group_desc *gd, unsigned char blk_map_ptr[], char *name) {
    int free_blk_ind;
    if ((free_blk_ind = get_free_bitmap(sb, gd, blk_map_ptr, sb->s_blocks_count)) < 0) {
        return -1;
    }
    sb->s_free_blocks_count --;
    gd->bg_free_blocks_count --;

    inode_tbl[new_i].i_block[0] = free_blk_ind + 1;
    struct ext2_dir_entry_2 *de = (struct ext2_dir_entry_2 *)(disk + inode_tbl[new_i].i_block[0] * EXT2_BLOCK_SIZE);
    int metadata_size = sizeof(unsigned int) + sizeof(short) + sizeof(char) * 2 + strlen(".");
    metadata_size = round_up(metadata_size, 4);
    int me_dot = metadata_size;
    de->inode = new_i + 1;
    de->rec_len = metadata_size;
    de->name_len = strlen(".");
    de->file_type = EXT2_FT_DIR;
    strncpy(de->name, ".", strlen("."));

    de = (void *)de + metadata_size;

    metadata_size = sizeof(unsigned int) + sizeof(short) + sizeof(char) * 2 + strlen("..");
    metadata_size = round_up(metadata_size, 4);

    inode_tbl[par_i].i_links_count ++;

    de->inode = par_i + 1;
    de->rec_len = EXT2_BLOCK_SIZE - me_dot;
    de->name_len = strlen("..");
    de->file_type = EXT2_FT_DIR;
    strncpy(de->name, "..", strlen(".."));

    // ================================================================
    int isfound = 0;
    for (int j = 0; j < 15 && !isfound; j++) {
        if (j < 12) {
            struct ext2_dir_entry_2 *curr_de = (struct ext2_dir_entry_2 *)(disk + inode_tbl[par_i].i_block[j] * EXT2_BLOCK_SIZE);
            struct ext2_dir_entry_2 *prev_de = NULL;
            if (inode_tbl[par_i].i_block[j]) {
                int curr_entry = 0;
                int newf_metadata_size = sizeof(unsigned int) + sizeof(short) + sizeof(char) * 2 + strlen(name);
                while (curr_entry < EXT2_BLOCK_SIZE) {
                    int metadata_size = sizeof(unsigned int) + sizeof(short) + sizeof(char) * 2 + curr_de->name_len;
                    metadata_size = round_up(metadata_size, 4);
                    int remaining_bytes = curr_de->rec_len - (metadata_size);
                    if ( remaining_bytes >= newf_metadata_size) {
                        // If it is marked as deleted
                        // WARNING: repetitive
                        if (inode_tbl[curr_de->inode].i_dtime) {
                            prev_de = curr_de;
                            curr_de->inode = new_i + 1;
                            curr_de->rec_len = prev_de->rec_len - metadata_size;
                            curr_de->name_len = strlen(name);
                            curr_de->file_type = EXT2_FT_DIR;
                            strncpy(curr_de->name, name, strlen(name));
                            prev_de->rec_len = metadata_size;

                        } else {

                            prev_de = curr_de;
                            curr_de = (void *)curr_de + metadata_size;
                            curr_de->inode = new_i + 1;
                            curr_de->rec_len = prev_de->rec_len - metadata_size;
                            curr_de->name_len = (unsigned char)strlen(name);
                            curr_de->file_type = EXT2_FT_DIR;
                            strncpy(curr_de->name, name, strlen(name));
                            printf("inode %d rec %d name_len %d file_typ %d name %s\n", curr_de->inode,curr_de->rec_len,curr_de->name_len, curr_de->file_type, curr_de->name);
                            prev_de->rec_len = metadata_size;
                            printf("%d\n", metadata_size);
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
                if (( free_bb_pos = get_free_bitmap(sb, gd, blk_map_ptr, sb->s_blocks_count)) < 0) {
                    return -ENOSPC;
                }
                sb->s_free_blocks_count --;
                gd->bg_free_blocks_count --;
                inode_tbl[par_i].i_block[j] = free_bb_pos + 1;

                curr_de->inode = new_i + 1;
                curr_de->rec_len = EXT2_BLOCK_SIZE;
                curr_de->name_len = strlen(name);
                curr_de->file_type = EXT2_FT_DIR;
                strncpy(curr_de->name, name, strlen(name));
            }
        } else {
            // TODO: Implement the other one for j == 12 only
        }
    }

    // Update the bb_ptr;
    for (int i = 0; i < 16; i ++) {
        bb_ptr[i] = blk_map_ptr[i];
    }
    return 0;
}


// WARNING: duplicated!
void parse_cmd(int argc, char **argv, char **img, char **path) {
    char *usage = "Usage: ext2_mkdir <my_disk> <path_on_fs>\n";

    if (argc != 3) {
        fprintf(stderr, "%s", usage);
        exit(1);
    }

    *img = argv[1];
    *path = argv[2];

    /* Check whether the first character of abs_path start with root, if not
    print and error!*/
    if (*path[0] != '/') {
        fprintf(stderr, "ERROR: Absolute path should start with '/'\n");
        exit(1);
    }
}


int main(int argc, char **argv) {
    char *abs_path;

    parse_cmd(argc, argv, &img_name, &abs_path);

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

    init_block_pointrs();

    char *dir_name = NULL;
    int par_inode_ind = get_parent_ino(abs_path, &dir_name);
    if (par_inode_ind == -1) {
        fprintf(stderr, "%s already exist!\n", dir_name);
        return EEXIST;
    } else if (par_inode_ind == -2) {
        fprintf(stderr, "Not a directory!\n");
        return ENOENT;
    }

    /* NOTE: Copy of the block bitmap*/
    unsigned char block_bitmap_copy[sb->s_blocks_count / 8];
    for (int i = 0; i < 16; i++) {
        block_bitmap_copy[i] = bb_ptr[i];
    }

    int curr_inode_ind = create_inode_dir(inode_tbl, ib_ptr, sb, gd, block_bitmap_copy);
    if (curr_inode_ind < 0) {
        fprintf(stderr, "Not enough space!\n");
        return ENOSPC;
    }

    create_dir(inode_tbl, ib_ptr, bb_ptr, par_inode_ind, curr_inode_ind, sb, gd, block_bitmap_copy, dir_name);
    // If ( < )
    // {ENOSPC}




    return 0;
}
