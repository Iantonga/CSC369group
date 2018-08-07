#include <assert.h>
#include <errno.h>
#include <time.h> /* For timestamp */
#include <getopt.h> /* For getting the required cmd line flags*/
#include <time.h> /* For timestamp */
#include "ext2_util.h"


struct node {
    struct ext2_dir_entry_2 *de;
    struct node *next;
};

struct linkedlist {
    struct node *front;
    struct node *back;
};

void append(struct linkedlist *l, struct ext2_dir_entry_2 *de) {
    struct node *new_node = (struct node *)malloc(sizeof(struct node));
    new_node->next = NULL;
    new_node->de = de;
    if (l->front == NULL) {
        assert(!l->back); // Back should not be NULL either.
        l->front = new_node;
        l->back = new_node;
    } else {
        assert(l->back);
        l->back->next = new_node;
        l->back = new_node;
    }
}

struct linkedlist *get_entries(int inode_i) {
    struct linkedlist *list = (struct linkedlist *)malloc(sizeof(struct linkedlist));
    list->front = NULL;
    list->back = NULL;
    for (int j = 0; j < 15; j++ ) {
        if (j < 12) {
            if (inode_tbl[inode_i].i_block[j]) {

                struct ext2_dir_entry_2 *de = (struct ext2_dir_entry_2 *)(disk
                    + inode_tbl[inode_i].i_block[j] * EXT2_BLOCK_SIZE);
                int curr_entry = 0;
                if (j == 0) { // skip . and ..
                    curr_entry += de->rec_len;
                    de = (void *)de + de->rec_len;
                    curr_entry += de->rec_len;
                    de = (void *)de + de->rec_len;
                }
                while (curr_entry < EXT2_BLOCK_SIZE) {
                    append(list, de);
                    curr_entry += de->rec_len;
                    de = (void *)de + de->rec_len;
                }

            }
        } else {// TODO: j == 13
        }
    }
    return list;
}


void delete_entry(int inode_i, int parent_i) {
    int isfound = 0;
    printf("inode to be deleted: %d, the parent: %d\n", inode_i + 1, parent_i + 1);
    for (int j = 0; j < 15 && !isfound; j++ ) {
        if (j < 12) {
            if (inode_tbl[parent_i].i_block[j]) {
                struct ext2_dir_entry_2 *curr_de = (struct ext2_dir_entry_2 *)(disk
                    + inode_tbl[parent_i].i_block[j] * EXT2_BLOCK_SIZE);
                struct ext2_dir_entry_2 *prev_de = NULL;

                int curr_entry = 0;
                while (curr_entry < EXT2_BLOCK_SIZE) {
                    if (curr_de->inode == inode_i + 1) {
                        if (prev_de != NULL) {
                            prev_de->rec_len += curr_de->rec_len;
                        }
                        isfound = 1;
                        break;
                    } else {
                        prev_de = curr_de;
                        curr_entry += curr_de->rec_len;
                        curr_de = (void *)curr_de + curr_de->rec_len;
                    }
                }

            }
        } else {
            // TODO: implement this j == 12
        }
    }
}


void remove_file(char *path, int rflag, int parent_i,
        unsigned char blk_bitmap_ptr[], unsigned char inode_bitmap_ptr[]) {

    int inode_i = get_inode_from_path(path, NO);
    if (inode_i >= 0) {
        printf("here\n");
        if (inode_tbl[inode_i].i_mode & EXT2_S_IFDIR) { // If it is a directory annd flag is set recursively call!
            printf("lol1\n");
            if (rflag){
                struct linkedlist *entries = get_entries(inode_i);
                struct node *curr_node = entries->front;
                while (curr_node != NULL) {
                    printf("Inode: %d rec_len: %d name_len: %d type= %c name=%s\n", curr_node->de->inode,
                        curr_node->de->rec_len, curr_node->de->name_len, curr_node->de->file_type, curr_node->de->name);


                    char new_path[strlen(path) + curr_node->de->name_len + 2];
                    sprintf(new_path, "%s/%s", path, curr_node->de->name);
                    remove_file(new_path, rflag, inode_i, blk_bitmap_ptr, inode_bitmap_ptr);

                    curr_node = curr_node->next;
                }
                // remove the directory itself.
                inode_tbl[inode_i].i_dtime = (unsigned int)time(NULL);
                for (int b = 0; b < 15; b++){
                    if (inode_tbl[inode_i].i_block[b]) {
                        set_free_bitmap(inode_tbl[inode_i].i_block[b], blk_bitmap_ptr);
                        sb->s_free_blocks_count ++;
                        gd->bg_free_blocks_count ++;
                        gd->bg_used_dirs_count --;
                    }
                }

                delete_entry(inode_i, parent_i);
                set_free_bitmap(inode_i, inode_bitmap_ptr);
                sb->s_free_inodes_count ++;
                gd->bg_free_inodes_count ++;

            } else {
                fprintf(stderr, "Cannot remove a directory!Run rm with -r\n");
                exit(EISDIR);
            }

        } else { // This is a file!
            printf("lol2\n");
            inode_tbl[inode_i].i_dtime = (unsigned int)time(NULL);
            delete_entry(inode_i, parent_i);
            for (int b = 0; b < 15; b++){
                if (inode_tbl[inode_i].i_block[b]) {
                    set_free_bitmap(inode_tbl[inode_i].i_block[b], blk_bitmap_ptr);
                    sb->s_free_blocks_count ++;
                    gd->bg_free_blocks_count ++;
                }

            }
            set_free_bitmap(inode_i, inode_bitmap_ptr);
            sb->s_free_inodes_count ++;
            gd->bg_free_inodes_count ++;
        }

    } else {
        fprintf(stderr, "Not a directory or a file!\n");
        exit(ENOENT);
    }
}


/* WARNING: ls and rm share the same code*/
void parse_cmd(int argc, char **argv, int *flag_r, char **img, char **path) {
    int opt;
    char *usage = "Usage: ext2_rm [-r] <my_disk> <path_on_fs>\n";

    while (optind < argc) {
        int tmp = optind;
        while ((opt = getopt(argc, argv, "r")) != -1) {
    		switch (opt) {
        		case 'r':
                    *flag_r = 1;
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
            if (!*flag_r) {
                fprintf(stderr, "%s", usage);
                exit(1);
            }
        } else {
            fprintf(stderr, "%s", usage);
            exit(1);
        }

        /* Eaxample: ./ext2_ls img -a*/
    } else if (argc == 3 && *flag_r) {
        fprintf(stderr, "%s", usage);
        exit(1);
    }

    /* Now that we passed all the erros we set the img_name and abs_path
    to their appropriate values*/
    if (*flag_r) {
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
    int rflag = FALSE;
    char *img_name = NULL;
    char *path = NULL;
    parse_cmd(argc, argv, &rflag, &img_name, &path);

    int fd = open(img_name, O_RDWR);
    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
    	perror("mmap");
    	exit(1);
    }

    init_block_pointrs();

    unsigned char block_bitmap_copy[sb->s_blocks_count / 8];
    for (int i = 0; i < (sb->s_blocks_count/8); i++) {
        block_bitmap_copy[i] = bb_ptr[i];
    }

    unsigned char inode_bitmap_copy[sb->s_inodes_count / 8];
    for (int i = 0; i < (sb->s_inodes_count/8); i++) {
        inode_bitmap_copy[i] = ib_ptr[i];
    }


    int inode = get_inode_from_path(path, NO);
    if (inode < 0) {
        fprintf(stderr, "No entry!\n");
        return ENOENT;
    } else if (inode == 1) {
        fprintf(stderr, "Cannot remove root!\n");
        return EPERM;
    }

    char *parent_dirs;
    if ((parent_dirs = dirname(path)) == NULL) {
        perror("dirname");
        return EPERM;
    }

    int parent_inode = get_inode_from_path(parent_dirs, NO);
    if (inode < 0) {
        fprintf(stderr, "No entry!\n");
        return ENOENT;
    }

    // test with /. /.. /level1/. /level1/..
    remove_file(path, rflag, parent_inode, block_bitmap_copy, inode_bitmap_copy);


    // printf("\nbitmap: ");
    // dude(block_bitmap_copy, sb->s_blocks_count / 8);
    //
    // set_free_bitmap(23, block_bitmap_copy, sb->s_blocks_count);
    //
    // printf("\nbitmap: ");
    // dude(block_bitmap_copy, sb->s_blocks_count / 8);
    for (int i = 0; i < (sb->s_blocks_count/8); i++) {
        bb_ptr[i] = block_bitmap_copy[i];
    }

    for (int i = 0; i < (sb->s_inodes_count/8); i++) {
        ib_ptr[i] = inode_bitmap_copy[i];
    }
}
