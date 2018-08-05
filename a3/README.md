./ext2_cp images/e.img  cp_test/block001  /


Quesitons:
    1. What if something goes wrong between updating the inode and actually putting a directory directory entry

    2. In ext2_cp when we copy a file and a new block needs to be allocated for the directory that is containing the file because the previous blocks could not fit in
    the directory entry? So what to do in that case?

    3. What if in ext2_cp at line 432, the file that is overwriting is really
    huge and a new block is needed?
        ans: I think that is taken care of in copy_file

    4. Should sb and gd be updated after inode created or after directory entry recorded?


    5. why in readimage_sol when sb->s_inodes_count and sb->s_blocks_count is odd we
    have 8 bits less in the bitmap?
        ans: it's because of the integer division int he for loop!


    6. Number of links after mkdir is 2 because of the '.' and parent/new_dir 
