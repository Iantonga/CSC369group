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


void parse_cmd(int argc, char **argv, int *flag_a, char **img, char **path) {
    int opt;
    char *usage = "Usage: ext2_cp <my_disk> <path_on_my_computer> <path_on_fs>\n";


    if (argc != 4) {
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
