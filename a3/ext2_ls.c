#include <getopt.h> /* For getting the required arguments*/
#include "ext2_util.h"


/*
List entries of directory that begin at the jth
block pointer of that directory's corresponding inode.
block_num is the block where the data begins, and
aflag indicates whether to list the current directory
and the parent directory (. and ..) in addition to the entries.
*/
void list_entries(int block_num, int j, int aflag){
    struct ext2_dir_entry_2 *de = (struct ext2_dir_entry_2 *)(disk +
        block_num * EXT2_BLOCK_SIZE);

    int curr_entry = 0;

    // if the aflag is used then don't skip the first block with . and ..
    if (!aflag && j == 0) {
        curr_entry += de->rec_len;
        de = (void *)de + de->rec_len;

        curr_entry += de->rec_len;
        de = (void *)de + de->rec_len;
    }

    while (curr_entry < EXT2_BLOCK_SIZE) {
        char tmp_name[(int) de->name_len];
        memset(tmp_name, 0, de->name_len + 1);
        strncpy(tmp_name, de->name, de->name_len);
        if (de->name_len != 0) {
            if (de->file_type == EXT2_FT_DIR) {
                printf("%s/\n", tmp_name);
            } else {
                printf("%s\n", tmp_name);
            }
        }
        curr_entry += de->rec_len;
        de = (void *)de + de->rec_len;

    }
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
    img_name = NULL;
    char *abs_path = NULL;

    parse_cmd(argc, argv, &aflag, &img_name, &abs_path);

    int fd = open(img_name, O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
    	perror("mmap");
    	exit(1);
    }

    init_block_pointrs();

    int inode_index = get_inode_from_path(abs_path, YES);
    if (inode_index < 0) {
        fprintf(stderr, "No such file or directory\n");
        return ENOENT;
    }


   /* If the path leads to it[i] is either directory then list the contents of
   that directory o.w. just list that.*/
   if (inode_tbl[inode_index].i_mode & EXT2_S_IFDIR) {
       for (int j = 0; j < 15; j++) {
           if (j < 12) {
               if (inode_tbl[inode_index].i_block[j]) {
                   list_entries(inode_tbl[inode_index].i_block[j], j, aflag);
               }
           } else if (j == 12) {
               unsigned int *s_indir = (unsigned int *)(disk +
                   inode_tbl[inode_index].i_block[j] * EXT2_BLOCK_SIZE);
                for (int s = 0; s < (EXT2_BLOCK_SIZE / sizeof(unsigned)); s++) {
                   if (s_indir[s]) {
                       list_entries(s_indir[s], j, aflag);
                   }
               }
           }
       }
   }

    return 0;
}
