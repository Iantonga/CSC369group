# FLAGS = -Wall -g -std=gnu99
FLAGS = -Wall
DEPENDENCIES = ext2.h
EXEC = ext2_ls
# EXEC = ext2_ls.o

all : $(EXEC)

ext2_ls : ext2_ls.c $(DEPENDENCIES)
	gcc $(FLAGS) -o $@ $<

# ext2_cp : ext2_cp.c $(DEPENDENCIES)
# 	gcc $(FLAGS) -o $@ $<
#
# ext2_mkdir : ext2_mkdir.c $(DEPENDENCIES)
# 	gcc $(FLAGS) -o $@ $<
#
# ext2_ln : ext2_ln.c $(DEPENDENCIES)
# 	gcc $(FLAGS) -o $@ $<
#
# ext2_rm : ext2_rm.c $(DEPENDENCIES)
# 	gcc $(FLAGS) -o $@ $<
#
# %.o : %.c $(DEPENDENCIES)
# 	gcc $(FLAGS) -c $<

clean :
	rm -f $(EXEC)
