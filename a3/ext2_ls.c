#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h> /* Added this for strncpy*/
#include <getopt.h> /* For getting the required arguments*/
#include "ext2.h"

#define FALSE 0
#define TRUE 1

unsigned char *disk;

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
                    *flag_a = TRUE;
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
            if (*flag_a == FALSE) {
                fprintf(stderr, "%s", usage);
                exit(1);
            }
        } else {
            fprintf(stderr, "%s", usage);
            exit(1);
        }

        /* Eaxample: ./ext2_ls img -a*/
    } else if (argc == 3 && *flag_a == TRUE) {
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

}


int main(int argc, char **argv) {
    int aflag = FALSE;
    char *img_name = NULL;
    char *abs_path = NULL;
    parse_cmd(argc, argv, &aflag, &img_name, &abs_path);
    printf("Hello There! I have received: img:%s path:%s flag:%d\n", img_name, abs_path, aflag);


    return 0;
}
