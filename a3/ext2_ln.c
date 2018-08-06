#include <assert.h>
#include <errno.h>
#include <time.h> /* For timestamp */
#include <getopt.h> /* For getting the required cmd line flags*/
#include "ext2_util.h"

void parse_cmd(int argc, char **argv, int *flag_a, char **img, char **path,
        char **link_path) {

    int opt;
    char *usage = "Usage: ext2_ln [-s] <my_disk> <path_on_fs> <link_path_on_fs> \n";

    while (optind < argc) {
        int tmp = optind;
        while ((opt = getopt(argc, argv, "a")) != -1) {
    		switch (opt) {
        		case 'a':
                    *flag_a = 1;
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
            if (!*flag_a) {
                fprintf(stderr, "%s", usage);
                exit(-1);
            }
        } else {
            fprintf(stderr, "%s", usage);
            exit(-1);
        }

        /* Eaxample: ./ext2_ls img -a*/
    } else if (argc == 4 && *flag_a) {
        fprintf(stderr, "%s", usage);
        exit(-1);
    }

    if (*flag_a) {
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
int helper(char *path, char *link_path, int aflag, int *inode_src, int *inode_dest, char **link_name) {
    char *fname_src;
    char *fname_dest;
    char *dir_dest;
    if ((fname_src = basename(path)) == NULL) {
        perror("basename");
        exit(1);
    }

    if ((fname_dest = basename(link_path)) == NULL) {
        perror("basename");
        exit(1);
    }

    if (((dir_dest) = dirname(dir_dest)) == NULL) {
        perror("dirname");
        exit(1);
    }

    if ((*inode_src = get_inode_from_path(path, NO)) >= 0) {
        /*WARNING: soft links can link a directory i think but the assignment
        says something different*/
        if (inode_tbl[*inode_src].i_mode & EXT2_S_IFDIR) {
            if (!aflag) {
                return -3;

            } else {
                // TODO: soft linking a dir.
                printf("soft link bruh!\n");
            }

        } else if(inode_tbl[*inode_src].i_mode & EXT2_FT_REG_FILE ||
                inode_tbl[*inode_src].i_mode & EXT2_FT_SYMLINK){
            if ((*inode_dest = get_inode_from(link_path, NO)) >= 0) {
                if(inode_tbl[inode_dest].i_mode & EXT2_FT_DIR) {
                    char *new_path = concat_to_path(link_path, fname_src);
                    printf("after concat %s, result:%s\n", link_path, new_path);
                    if (get_inode_from(new_path, NO) >= 0) {
                        printf("It already exists after cat\n");
                        return -4;
                    }

                } else {
                    return -4;
                }
            } else if ((inode_dest = get_inode_from(dir_dest, NO)) < 0)) {
                return -2;
            }
            *link_name = fname_dest;
            printf("making a new link at %s with name %s\n", link_path, fname_dest);

        } else {
            fprintf(stderr, "Can only link a ref file or a symbolic link\n");
            exit(1);
        }
    } else {
        return -2;
    }

    return 1; // successful!
}


int main(int argc, char **argv) {
    int aflag = FALSE;
    char *abs_path = NULL;
    char *link_abs_path = NULL;

    parse_cmd(argc, argv, &aflag, &img_name, &abs_path, &link_abs_path);

    int fd = open(img_name, O_RDWR);
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
    	perror("mmap");
    	exit(1);
    }

    init_block_pointrs();

    int inode_src, inode_dest;
    char *link_name;
    int status = helper(abs_path, link_path, aflag, &inode_src, &inode_dest, &link_name);
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


    return 0;
}
