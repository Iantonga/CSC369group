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
#include <errno.h>
#include "ext2.h"

unsigned char *disk;


void copy_file_test(FILE *fsrc, FILE *fdest) {
    char data;
	while (fread(&data, sizeof(char), 1, fsrc) == 1) {
		if (fwrite(&data, sizeof(char), 1, fdest) < 1) {
			perror("fwrite");
			exit(-1);
		}
	}
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
    // if (*ext2_path[0] != '/') {
    //     fprintf(stderr, "ERROR: Absolute path should start with '/'\n");
    //     exit(1);
    // }
}


int main(int argc, char **argv) {
    char *img_name;
    char *os_path;
    char *ext2_path;
    char *fsrc_name;
    FILE *fsrc;

    parse_cmd(argc, argv, &img_name, &os_path, &ext2_path);

    // Opening the image file.
    // int fd = open(argv[1], O_RDWR);
    // if (fd == -1) {
    //     perror("open");
    //     exit(1);
    // }
    // disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    // if(disk == MAP_FAILED) {
    // 	perror("mmap");
    // 	exit(1);
    // }

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


    char new_dest[strlen(ext2_path) + 1 + strlen(fsrc_name) + 1];
    if (sprintf(new_dest, "%s/%s", ext2_path, fsrc_name) < 0) {
        perror("sprintf");
        exit(-1);
    }
    FILE *fdest = fopen(new_dest, "r+b");
    if (fdest == NULL) {
        // If it doesn't exist make a new file.
        if (errno == ENOENT) {
            fdest = fopen(new_dest, "wb");
            if (fdest == NULL) {
                perror("fopen");
                exit(-1);
            }
            copy_file_test(fsrc, fdest);

        } else {
            perror("fopen");
            exit(-1);
        }

    } else {
        // If the file already exists.
        copy_file_test(fsrc, fdest);


    }





    return 0;
}
