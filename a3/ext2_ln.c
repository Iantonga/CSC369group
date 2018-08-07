#include <assert.h>
#include <errno.h>
#include <time.h> /* For timestamp */
#include <getopt.h> /* For getting the required cmd line flags*/
#include <time.h> /* For timestamp */
#include "ext2_util.h"

void parse_cmd(int argc, char **argv, int *flag_s, char **img, char **path,
        char **link_path) {

    int opt;
    char *usage = "Usage: ext2_ln [-s] <my_disk> <path_on_fs> <link_path_on_fs> \n";

    while (optind < argc) {
        int tmp = optind;
        while ((opt = getopt(argc, argv, "s")) != -1) {
    		switch (opt) {
        		case 's':
                    *flag_s = 1;
        			break;
        		default:
                    fprintf(stderr, "%s", usage);
                    exit(-1);

    		}
    	}
        /*Check whether optind been incremented(if yes then we found the arg,
        o.w keep looking until we hit argc)*/
        if (tmp == optind) {
            optind ++;
        } else {
            break;
        }
    }

    if (argc != 4) {
        if (argc == 5) {
            if (!*flag_s) {
                fprintf(stderr, "%s", usage);
                exit(-1);
            }
        } else {
            fprintf(stderr, "%s", usage);
            exit(-1);
        }

        /* Eaxample: ./ext2_ls img -a*/
    } else if (argc == 4 && *flag_s) {
        fprintf(stderr, "%s", usage);
        exit(-1);
    }

    if (*flag_s) {
        int flag_location = optind - 1;
        if (flag_location == 1) {
            *img = argv[2];
            *path = argv[3];
            *link_path = argv[4];
        } else if (flag_location == 2) {
            *img = argv[1];
            *path = argv[3];
            *link_path = argv[4];
        } else if (flag_location == 3) {
            *img = argv[1];
            *path = argv[2];
            *link_path = argv[4];
        } else {
            *img = argv[1];
            *path = argv[2];
            *link_path = argv[3];
        }
    } else {
        *img = argv[1];
        *path = argv[2];
        *link_path = argv[3];
    }

    /* Check whether the first character of abs_path start with root, if not
    print and error!*/
    if (*path[0] != '/' || *link_path[0] != '/') {
        fprintf(stderr, "ERROR: Absolute path should start with '/'\n");
        exit(1);
    }

}


/* Return -1 to indicate ENOENT (a file treated as a directory)
   Return -2 to indicate ENOENT (no entry)
   Return -3 to indicate EISDIR
   Return -4 to indicate EEXIST*/
int set_inode_indices(char *path, char *link_path, int sflag, int *inode_src, int *inode_dest, char **link_name) {
    // WARNING: Since basename returns a pointer and it'll be overwritten in the
    // subsequent calls.


    char *basename_ptr;
    char *dir_dest;

    if ((basename_ptr = basename(path)) == NULL) {
        perror("basename");
        exit(1);
    }

    char fname_src[strlen(basename_ptr)];
    sprintf(fname_src, "%s", basename_ptr);
    fname_src[strlen(basename_ptr)] = '\0';

    if ((basename_ptr = basename(link_path)) == NULL) {
        perror("basename");
        exit(1);
    }
    char fname_dest[strlen(basename_ptr)];
    sprintf(fname_dest, "%s", basename_ptr);
    fname_dest[strlen(basename_ptr)] = '\0';

    if ((dir_dest = dirname(link_path)) == NULL) {
        perror("dirname");
        exit(1);
    }

    if ((*inode_src = get_inode_from_path(path, NO)) >= 0) {
        /*WARNING: soft links can link a directory i think but the assignment
        says something different*/
        if (inode_tbl[*inode_src].i_mode & EXT2_S_IFDIR) {
            // TODO: might need to change this!
            if (!sflag) {
                return -3;

            } else {
                fprintf(stderr, "Not implemented for this assignment!\n");
                return -3;
            }
        } else if(inode_tbl[*inode_src].i_mode & EXT2_S_IFREG ||
                inode_tbl[*inode_src].i_mode & EXT2_S_IFLNK){
            if ((*inode_dest = get_inode_from_path(link_path, NO)) >= 0) {
                if(inode_tbl[*inode_dest].i_mode & EXT2_S_IFDIR) {
                    char *new_path = concat_to_path(link_path, fname_src);
                    if (get_inode_from_path(new_path, NO) >= 0) {
                        free(new_path);
                        return -4;
                    }
                    free(new_path);
                    *link_name = malloc(strlen(fname_src) + 1);
                    strncpy(*link_name, fname_src, strlen(fname_src));
                    (*link_name)[strlen(fname_src)] = '\0';


                } else {
                    // The file already exists
                    printf("here\n");
                    return -4;
                }
            } else if ((*inode_dest = get_inode_from_path(dir_dest, NO)) < 0) {
                return -2;
            } else {
                if (link_path[strlen(link_path) - 1] == '/') {
                    // Since the last directory cannot be a directory
                    return -2;
                }
                *link_name = malloc(strlen(fname_dest) + 1);
                strncpy(*link_name, fname_dest, strlen(fname_dest));
                (*link_name)[strlen(fname_dest)] = '\0';
            }



        } else {
            fprintf(stderr, "Can only link a reg file or a symbolic link\n");
            exit(1);
        }
    } else {
        return -2;
    }

    return 1; // successful!
}

void create_entry(int inode_src, int inode_dest, int sflag, char *path, char *fname) {
    // WARNING: duplicated code
    int isfound = 0;
    for (int j = 0; j < 15 && !isfound; j++) {
        if (j < 12) {
            struct ext2_dir_entry_2 *curr_de = (struct ext2_dir_entry_2 *)(disk + inode_tbl[inode_dest].i_block[j] * EXT2_BLOCK_SIZE);
            struct ext2_dir_entry_2 *prev_de = NULL;
            if (inode_tbl[inode_dest].i_block[j]) {
                int curr_entry = 0;
                int newf_metadata_size = sizeof(unsigned int) + sizeof(short) + sizeof(char) * 2 + strlen(fname);
                while (curr_entry < EXT2_BLOCK_SIZE) {
                    int metadata_size = sizeof(unsigned int) + sizeof(short) + sizeof(char) * 2 + curr_de->name_len;
                    metadata_size = round_up(metadata_size, 4);
                    int remaining_bytes = curr_de->rec_len - (metadata_size);
                    if ( remaining_bytes >= newf_metadata_size) {
                        if (inode_tbl[curr_de->inode].i_dtime) {

                            if (newf_metadata_size <= metadata_size) {
                                curr_de->inode = inode_src + 1;
                                if (prev_de != NULL) {
                                    curr_de->rec_len = prev_de->rec_len;
                                }
                                curr_de->name_len = strlen(fname);
                                if (sflag)  {
                                    curr_de->file_type = EXT2_FT_SYMLINK;
                                } else {
                                    curr_de->file_type = EXT2_FT_REG_FILE;
                                }
                                strncpy(curr_de->name, fname, strlen(fname));
                                if (prev_de != NULL) {
                                    prev_de->rec_len = metadata_size;
                                }

                            }
                        } else {
                            prev_de = curr_de;
                            curr_de = (void *)curr_de + metadata_size;
                            curr_de->inode = inode_src + 1;
                            curr_de->rec_len = prev_de->rec_len - metadata_size;
                            curr_de->name_len = strlen(fname);
                            if (!sflag) {
                                curr_de->file_type = EXT2_FT_REG_FILE;
                            } else {
                                curr_de->file_type = EXT2_FT_SYMLINK;
                            }

                            strncpy(curr_de->name, fname, strlen(fname));
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
                if (( free_bb_pos = get_free_bitmap(sb, gd, bb_ptr, sb->s_blocks_count)) < 0) {
                    exit(ENOSPC);
                }
                inode_tbl[inode_dest].i_block[j] = free_bb_pos + 1;

                curr_de->inode = inode_src + 1;
                curr_de->rec_len = EXT2_BLOCK_SIZE;
                curr_de->name_len = strlen(fname);
                if (!sflag) {
                    curr_de->file_type = EXT2_FT_REG_FILE;
                } else {
                    curr_de->file_type = EXT2_FT_SYMLINK;
                }

                strncpy(curr_de->name, fname, strlen(fname));
            }
        } else {
            // TODO: j == 12;
        }

    }

}


int put_path_to_blocks(int pos, char *path, unsigned char blk_map_ptr[]) {
    unsigned char *block_ptr;
    int path_size = strlen(path);
    int new_pos = get_free_bitmap(sb, gd, blk_map_ptr, sb->s_blocks_count);
    if (new_pos < 0) {
        return -1;
    }
    block_ptr = disk + EXT2_BLOCK_SIZE * (new_pos + 1);

    if (path_size > EXT2_BLOCK_SIZE) {
        fprintf(stderr, "Too long ;)!\n");
        exit(ENAMETOOLONG);
    }
    strncpy((char *) block_ptr, path, path_size);

    return 0;
}

int create_soft_inode(unsigned char blk_map_ptr[], unsigned char inode_map_ptr[], char *path) {
    int pos;
    if (( pos = get_free_bitmap(sb, gd, ib_ptr, sb->s_inodes_count)) < 0) {
        return -1;
    }
    sb->s_free_inodes_count --;
    gd->bg_free_inodes_count -- ;

    memset(&inode_tbl[pos], 0, sizeof(inode_tbl[pos]));
    inode_tbl[pos].i_mode |= EXT2_S_IFLNK;
    // inode_tbl[pos].i_mode |= 0x002;
    // inode_tbl[pos].i_mode |= 0x004;
    inode_tbl[pos].i_atime = (unsigned int)time(NULL);
    inode_tbl[pos].i_ctime = (unsigned int)time(NULL);
    inode_tbl[pos].i_mtime = (unsigned int)time(NULL);
    if (put_path_to_blocks(pos, path, blk_map_ptr) < 0) {
        return -1;
    }
    inode_tbl[pos].i_size = strlen(path);
    inode_tbl[pos].i_blocks = my_ceil(inode_tbl[pos].i_size / 512);
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

int make_link(int inode_src, int inode_dest, int sflag, char *path, char *fname) {
    // if it is a hard link just move to that inode_tbl[inode_src] and
    // update the i_links_count.
    if (!sflag) {
        inode_tbl[inode_src].i_links_count ++;
        inode_tbl[inode_src].i_atime = (unsigned int)time(NULL);
        inode_tbl[inode_src].i_mtime = (unsigned int)time(NULL);
        create_entry(inode_src, inode_dest, sflag, path, fname);

    } else {
        unsigned char block_bitmap_copy[sb->s_blocks_count / 8];
        for (int i = 0; i < 16; i++) {
            block_bitmap_copy[i] = bb_ptr[i];
        }

        unsigned char inode_bitmap_copy[sb->s_inodes_count / 8];
        for (int i = 0; i < 4; i++) {
            block_bitmap_copy[i] = ib_ptr[i];
        }
        int pos;
        if ((pos = create_soft_inode(block_bitmap_copy, inode_bitmap_copy, path)) == -1) {
            return -1;
        }
        create_entry(pos, inode_dest, sflag, path, fname);
        // SET THE ib_ptr and bb_ptr to the actual bits
    }
    return 0;
}



int main(int argc, char **argv) {
    int sflag = FALSE;
    char *abs_path = NULL;
    char *link_abs_path = NULL;

    parse_cmd(argc, argv, &sflag, &img_name, &abs_path, &link_abs_path);

    int fd = open(img_name, O_RDWR);
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
    	perror("mmap");
    	exit(1);
    }

    init_block_pointrs();

    int inode_src, inode_dest;
    char *link_name = NULL;
    // TODO: rename helper!
    int status = set_inode_indices(abs_path, link_abs_path, sflag, &inode_src, &inode_dest, &link_name);
    if (status == -1 || status == -2) {
        fprintf(stderr, "No entry\n");
        return ENOENT;
    } else if (status == -3) {
        fprintf(stderr, "hard link not allowed for directory\n");
        return EISDIR;

    } else if (status == -4) {
        fprintf(stderr, "The file already exists!");
        return EEXIST;
    }
    printf("%s\n", link_name);
    make_link(inode_src, inode_dest, sflag, link_abs_path, link_name);

    return 0;
}
