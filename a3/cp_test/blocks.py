import sys

BLOCK_SIZE = 1024

def main(b):
    if (b <= 0):
        print "no_of_blocks should be greater than 0."
        exit(1)

    suffix = str(b)
    file_name = "block"
    if (b < 10):
        file_name += "00" + suffix
    elif (b < 100):
        file_name += "0" + suffix
    else:
        file_name += suffix


    size = b * BLOCK_SIZE - 2
    f = open(file_name, "w")
    print >>f,"s"+ ("z" * (size - 1)) + "e"


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print "Usage: python blocks.py <no_of_blocks>"
        exit(1)

    main(int(sys.argv[1]))
