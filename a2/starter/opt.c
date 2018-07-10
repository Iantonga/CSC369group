#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include "sim.h"

/* The hashtable size is set to the maximum possible amount of virtual
   pages to best prevent collision */
#define HSIZE PTRS_PER_PGDIR * PTRS_PER_PGTBL

extern unsigned memsize;

extern int debug;

extern struct frame *coremap;

/* We access physmem to get the virdual */
extern char *physmem;
extern char *tracefile;

/* This implementation uses a hashtable with the virtual address being the key
 * and a linked list of the indicies of access of that address being the value,
 * with the earliest access index being the front of the list.
*/

/* The current line in the trace file*/
int current_trace;

/* A node in the linked list stored in the hashtable*/
struct Node {
    int trace;
    struct Node *next;
};

/* The hash table entry where it stores the vaddr and a pointer to a Linked-list*/
struct hash_tbl_entry {
    addr_t vaddr;

    // Linked List attributes.
    struct Node *head;
    struct Node *back;
};

/* Hash table is an array of pointers of hash_tbl_enties*/
struct hash_tbl_entry *hash_table[HSIZE];

/* ================== START: LinkedList Functions ====================== */

/* Append to the linked-list that the hash_table[index] is pointing to.*/
void append(int index) {
    // Create a new node!
    struct Node *new_node = (struct Node*)malloc(sizeof(struct Node));
    new_node->trace = current_trace;
    new_node->next = NULL;

    // List is empty then back == head
    if (hash_table[index]->head == NULL) {
        hash_table[index]->head = new_node;
        hash_table[index]->back = new_node;
    } else {
        // The back of the list better not be NULL;
        assert(hash_table[index]->back);
        hash_table[index]->back->next = new_node;
        hash_table[index]->back = new_node;
    }
}

/* Delete the front of the linked-list that the hash_table[index] is
* pointing to.*/
void delete(int index) {
    // Front should not be NULL!
    assert(hash_table[index]->head);
    // Point to the front so that we can free it later
    struct Node *deleted_node = hash_table[index]->head;

    // If there is only one node in the list!
    if (hash_table[index]->head == hash_table[index]->back) {
        hash_table[index]->back = NULL;
    }
    hash_table[index]->head = hash_table[index]->head->next;

    deleted_node->next = NULL;
    free(deleted_node);
}

/* ================== END: LinkedList Functions ============ */

/* ================== START: Hash Table Functions ============ */

/* The hash function we employ uses a multiplier that is in the same order
 * of the key entry while having no common factors with HSIZE, this ensures
 * an even distribution of data in the hashtable to minimize conflict
 */
int hash(addr_t vaddr) {
	int multiplier;
	if (PTRS_PER_PGDIR == 4096) {
		multiplier = 16000057;
	} else {
		multiplier = 1000003;
	}
	return (vaddr * multiplier) % (HSIZE);
}

/* Insert to the hash table hash_table with given parameter virtual address that
 * is shifted to the right by PAGE_SHIFT (i.e vaddr >> PAGE_SHIFT).
 *
 * Make a new hash_tbl_entry if the hash_table is NULL for that vaddr (probing is
 * used if there is collision which is unlikely because our function is uniform
 * and our hash_table is large enough to store the maximum number of pages, 2^24
 * you could have.)
 *
 * If vaddr already has a entry in the hash table then append current_trace
 * to the linked-list.*/
void insert(addr_t vaddr) {
    // If the index returned by hash equals to NULL we put it there
    if (hash_table[hash(vaddr)] == NULL) {
        hash_table[hash(vaddr)] = (struct hash_tbl_entry*)malloc(1*sizeof(struct hash_tbl_entry));
        hash_table[hash(vaddr)]->vaddr = vaddr;
        hash_table[hash(vaddr)]->head = NULL;
        hash_table[hash(vaddr)]->back = NULL;
        append(hash(vaddr));

    } else {
        // This is to find the hash entry fir the corresponding vaddr to Append
        // to its linked-list if such a thing does not exist we .
        int i;
        for (i = 0; i < HSIZE; i++) {
            int index = (hash(vaddr) + i) % (HSIZE);
            if (hash_table[index] != NULL && hash_table[index]->vaddr == vaddr) {
                // Append to the hash_table[index]'s linked-list.
                append(index);
                break;

            // Probing to find an empty slot
            } else if (hash_table[index] == NULL) {
                hash_table[index] = (struct hash_tbl_entry*)malloc(1*sizeof(struct hash_tbl_entry));
                hash_table[index]->vaddr = vaddr;
                hash_table[index]->head = NULL;
                hash_table[index]->back = NULL;
                append(index);
                break;
            }
        }
        // This is just to check that the size of the hash_table is large enough!
        if (i == HSIZE) {
            fprintf(stderr, "hash table is not large enough! try running it with at least the number of lines in the trace file: for vaddr %lx\n", vaddr);
            exit(1);
        }

    }
}

/* Return the index of where given vaddr (i.e vaddr = v-address >> PAGE_SHIFT )
 * is located in the hash_table. Otherwise, return -1 if no such an entry exist
 */
int search(addr_t vaddr) {
    if (hash_table[hash(vaddr)] != NULL && hash_table[hash(vaddr)]->vaddr == vaddr) {
        return hash(vaddr);
    } else {
        // Do probing to find the wanted index for vaddr.
        for (int i = 0; i < HSIZE; i++) {
            int index = (hash(vaddr) + i) % HSIZE;
            if (hash_table[hash(vaddr)] != NULL && hash_table[index]->vaddr == vaddr) {
                return index;
            } else if (hash_table[index] == NULL) {
                return -1;
            }
        }
    }
    return -1;
}

/* ==================== END: Hash Table Functions ============= */



/* On eviction it goes through every hash entry of the pages currently on the
 * frame and finds the page with the largest first node with NULL being the max.
 * This means it will find the page that's never used again, and if not it finds
 * the page that's not used for the longest time. The lookup of every page in
 * frame and the subsequent comparison runs in time proportional to memsize.
 */
int opt_evict() {
	int max_trace = -1;
	int frame = -1;

  //Go through each page currently in frame
	for (int i = 0; i < memsize; i++) {
		char *mem_ptr = &physmem[i * SIMPAGESIZE];
		addr_t *vaddr_ptr = (addr_t *)(mem_ptr + sizeof(int));
		addr_t vaddr = *vaddr_ptr;
    //Find the page never used again
		int index = search(vaddr >> PAGE_SHIFT);
		if (hash_table[index]->head == NULL) {
			frame = i;
			return frame;
    //Or find one that's not used for the longest time
		} else {
			if (max_trace < hash_table[index]->head->trace) {
				max_trace = hash_table[index]->head->trace;
				frame = i;
			}
		}
	}
	return frame;
}

/* On reference it accesses the entry corresponding to p and deletes the
   first node in the linked list, indicated that it has past that index during
   the execution. This lookup and deletion runs in near constant time.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
	int frame_num = p->frame >> PAGE_SHIFT;
	char *mem_ptr = &physmem[frame_num * SIMPAGESIZE];
	addr_t *vaddr_ptr = (addr_t *)(mem_ptr + sizeof(int));
	addr_t vaddr = *vaddr_ptr;

	int index = search(vaddr >> PAGE_SHIFT);

	// Deletes the front of the linkedlist
	if (index != -1) {
		delete(index);
	} else {
    // In case the index wasn't found
        fprintf(stderr, "Unexpected index(%d) returned!\n", index);
        exit(1);
    }
	current_trace ++;
	return;
}

/* On init we initialize the hashtable and run through the lines of the trace
 * file, and add each entry to the hashtable. This runs in time near
 * porportional to the number of lines in the trace file as sometimes the insert
 * to the hastable will take longer due to probing.
 */
void opt_init() {
	current_trace = 0;
	// Initialize the hash table
	for (int i = 0; i < HSIZE; i++) {
		hash_table[i] = NULL;
	}

	// Open the file
	FILE *tfp = stdin;
	if (tracefile != NULL) {
		if((tfp = fopen(tracefile, "r")) == NULL) {
			perror("Error opening tracefile:");
			exit(1);
		}
	}

	// Read from the trace file!
	char buf[MAXLINE];
	addr_t vaddr = 0;
	// Just a placehodler
	char type;
	while(fgets(buf, MAXLINE, tfp) != NULL) {
    // Check for the valgrind comment lines
		if(buf[0] != '=') {
      // Locate the entry and insert to the hashtable
			sscanf(buf, "%c %lx", &type, &vaddr);
			insert(vaddr >> PAGE_SHIFT);
			current_trace ++;

		} else {
			continue;
		}
	}

	current_trace = 0;
}
