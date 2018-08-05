./ext2_cp images/e.img  cp_test/block001  /


Quesitons:
    1. What if something goes wrong between updating the inode and actually putting a directory directory entry

    2. In ext2_cp when we copy a file and a new block needs to be allocated for the directory that is containing the file because the previous blocks could not fit in
    the directory entry? So what to do in that case?

    3. What if in ext2_cp at line 432, the file that is overwriting is really
    huge and a new block is needed?
        ans: I think that is taken care of in copy_file
