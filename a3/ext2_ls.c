#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h> /* Added this for strncpy*/
#include <getopt.h> /* For getting the required arguments*/
#include <errno.h>
#include "ext2.h"


unsigned char *disk;

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
                } else if (b == 13) {
                    // TODO: Implement this
                } else if (b == 14) {
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


/*
* Parse the command line and set the required parameters to their appropriate
* values. The location of the flag '-a' does not matter. However, assume that
* name of the virtual image is passed in before the absolute path.
*/
void parse_cmd(int argc, char **argv, int *flag_a, char **img, char **path) {
    int opt;
    char *usage = "Usage: ext2_ls [-a] <my_disk> <path_on_fs>\n";

    while (optind < argc) {
        int tmp = optind;
        while ((opt = getopt(argc, argv, "a")) != -1) {
    		switch (opt) {
        		case 'a':
                    *flag_a = 1;
        			break;
        		default:
                    fprintf(stderr, "%s", usage);
                    exit(1);

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

    if (argc != 3) {
        if (argc == 4 ) {
            if (!*flag_a) {
                fprintf(stderr, "%s", usage);
                exit(1);
            }
        } else {
            fprintf(stderr, "%s", usage);
            exit(1);
        }

        /* Eaxample: ./ext2_ls img -a*/
    } else if (argc == 3 && *flag_a) {
        fprintf(stderr, "%s", usage);
        exit(1);
    }

    /* Now that we passed all the erros we set the img_name and abs_path
    to their appropriate values*/
    if (*flag_a) {
        int flag_location = optind - 1;
        if (flag_location == 1) {
            *img = argv[2];
            *path = argv[3];
        } else if (flag_location == 2) {
            *img = argv[1];
            *path = argv[3];
        } else {
            *img = argv[1];
            *path = argv[2];
        }
    } else {
        *img = argv[1];
        *path = argv[2];
    }

    /* Check whether the first character of abs_path start with root, if not
    print and error!*/
    if (*path[0] != '/') {
        fprintf(stderr, "ERROR: Absolute path should start with '/'\n");
        exit(1);
    }
}


int main(int argc, char **argv) {
    int aflag = 0;
    char *img_name = NULL;
    char *abs_path = NULL;

    parse_cmd(argc, argv, &aflag, &img_name, &abs_path);

    int fd = open(img_name, O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
    	perror("mmap");
    	exit(1);
    }

    int inode_index = get_inode_from_path(abs_path, 1);
    if (inode_index < 0) {
        return ENOENT;
    }


   struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
   unsigned int i_tbl_location = gd->bg_inode_table;
   struct ext2_inode *it = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * i_tbl_location);


   /* If the path lead to it[i] is either directory then list the contents of
   that directory o.w. just list that sing file.*/
   if (it[inode_index].i_mode & EXT2_S_IFDIR) {
       for (int j = 0; j < 15; j++) {
           if (j < 12) {
               if (it[inode_index].i_block[j]) {
                   struct ext2_dir_entry_2 *de = (struct ext2_dir_entry_2 *)(disk + it[inode_index].i_block[j] * EXT2_BLOCK_SIZE);
                   int curr_entry = 0;
                   if (!aflag) {
                       /* Skip .*/
                       curr_entry += de->rec_len;
                       de = (void *)de + de->rec_len;

                       /* Skip ..*/
                       curr_entry += de->rec_len;
                       de = (void *)de + de->rec_len;
                   }

                   while (curr_entry < EXT2_BLOCK_SIZE) {
                       // printf("curr_entry: %d\n", curr_entry);
                       // printf("curr_entry += rec: %d\n", curr_entry + de->rec_len);
                       char tmp_name[(int) de->name_len];
                       memset(tmp_name, 0, de->name_len + 1);
                       strncpy(tmp_name, de->name, de->name_len);
                       if (de->file_type == EXT2_FT_DIR) {
                           printf("%s/\n", tmp_name);
                       } else {
                           printf("%s\n", tmp_name);
                       }
                       curr_entry += de->rec_len;
                       de = (void *)de + de->rec_len;
                   }
               }
           } else {
               // TODO: Implement the other ones
           }
       }
   } else {
       // TODO: print the file only
   }

    return 0;
}
