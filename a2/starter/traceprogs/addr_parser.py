import sys




if __name__ == '__main__':
        if len(sys.argv) < 2:
                print "Error: no ref file was provided"
                exit(1)

        ref_fp = open(sys.argv[1], "r")
        
        add_to_num = {}


        for line in ref_fp:
                data = line.split(" ")
                page_num = hex(int(data[1], 16) >> 12)
                if page_num not in add_to_num:
                        add_to_num[page_num] = 1
                else:
                        add_to_num[page_num] += 1
          
	print "The numebr of frames: " len(add_to_num)
	print              
        for addr in add_to_num:
                print addr, add_to_num[addr]
        

        

        
        
	
