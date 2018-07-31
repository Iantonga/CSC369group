# FLAGS = -Wall -g -std=gnu99
FLAGS = -Wall -g
DEPENDENCIES = ext2.h
EXEC = ext2_ls

all : $(EXEC)

ext2_ls : ext2_ls.c $(DEPENDENCIES)
	gcc $(FLAGS) -o $@ $<


# %.o : %.c $(DEPENDENCIES)
# 	gcc $(FLAGS) -c $<

clean :
	rm -f $(EXEC)
