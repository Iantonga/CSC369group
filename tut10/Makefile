all: readimage readimage_sol

readimage : ext2.h readimage.c
	gcc -Wall -g -o readimage readimage.c

readimage_sol : ext2.h readimage_sol.c
	gcc -Wall -g -o readimage_sol readimage_sol.c

clean :
	rm -f readimage_sol
	rm -f readimage
