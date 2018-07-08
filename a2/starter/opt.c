#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"

// TODO: SHOULD WEEEEEE?????
#include "sim.h"


// #define HSIZE PTRS_PER_PGDIR * PTRS_PER_PGTBL
#define HSIZE 3000

extern unsigned memsize;

extern int debug;

extern struct frame *coremap;

extern char *physmem;
extern char *tracefile;

int current_trace;

struct Node {
    int trace;
    struct Node *next;
};

struct hash_tbl_entry {
    addr_t vaddr;

    // Linked List
    struct Node *head;
    struct Node *back;
};

struct hash_tbl_entry *hash_table[HSIZE];

/* ================== START: LinkedList Functions ============ */
void append(int index) {
    struct Node *new_node = (struct Node*)malloc(sizeof(struct Node));
    new_node->trace = current_trace;
    new_node->next = NULL;

    // List is empty
    if (hash_table[index]->head == NULL) {
        hash_table[index]->head = new_node;
        hash_table[index]->back = new_node;
    } else {
        //the back of the list better not be None;
        assert(hash_table[index]->back);
        hash_table[index]->back->next = new_node;
        hash_table[index]->back = new_node;
    }
}

void delete(int index) {
    // Front should not be None!
    assert(hash_table[index]->head);
    struct Node *deleted_node = hash_table[index]->head;
    if (hash_table[index]->head == hash_table[index]->back) {
        hash_table[index]->back = NULL;
    }
    hash_table[index]->head = hash_table[index]->head->next;

    deleted_node->next = NULL;
    free(deleted_node);
}

void print_list(int i) {
    struct Node *curr = hash_table[i]->head;
    if (curr != NULL) {
        printf("%d->", curr->trace);
        curr = curr->next;
    }

    while (curr) {
        printf("%d->", curr->trace);
        curr = curr->next;
    }
    printf("|\n");
}
/* ================== END: LinkedList Functions ============ */

/* ================== START: Hash Table Functions ============ */
/*TODO*/
int hash(addr_t vaddr) {
	int multiplier;
	if (PTRS_PER_PGDIR == 4096) {
		multiplier = 16000057;
	} else {
		multiplier = 1000003;
	}
	return (vaddr * multiplier) % (HSIZE);
}

void insert(addr_t vaddr) {
    // IF the index returned by hash equals to 0/NULL we put it there
    // If not we do probing (2 cases when probing we found a empty or we found
    // the vaddr)
    if (hash_table[hash(vaddr)] == NULL) {
        hash_table[hash(vaddr)] = (struct hash_tbl_entry*)malloc(1*sizeof(struct hash_tbl_entry));
        hash_table[hash(vaddr)]->vaddr = vaddr;
        hash_table[hash(vaddr)]->head = NULL;
        hash_table[hash(vaddr)]->back = NULL;
        append(hash(vaddr));

    } else {
        // PROBING (hash + i)
        int i;
        for (i = 0; i < HSIZE; i++) {
            int index = (hash(vaddr) + i) % HSIZE;
            if (hash_table[index] != NULL && hash_table[index]->vaddr == vaddr) {
                // printf("We are doing linkedlist operations for %lx!\n", vaddr);
                append(index);
                break;

            } else if (hash_table[index] == NULL) {
                hash_table[index] = (struct hash_tbl_entry*)malloc(1*sizeof(struct hash_tbl_entry));
                hash_table[index]->vaddr = vaddr;
                hash_table[index]->head = NULL;
                hash_table[index]->back = NULL;
                append(index);
                break;
            }
        }
        if (i == HSIZE) {
            fprintf(stderr, "hash table is not large enough! try running it with swapfilesize: for vaddr %lx\n", vaddr);
            exit(1);
        }

    }
}


int search(addr_t vaddr) {
    if (hash_table[hash(vaddr)] != NULL && hash_table[hash(vaddr)]->vaddr == vaddr) {
        return hash(vaddr);
    } else {
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


void print_hash_tbl() {
    printf("+---------------+\n");
    for (int i = 0; i < HSIZE; i++) {
        if (hash_table[i] != NULL) {
            printf("|%d, %lx|", i, hash_table[i]->vaddr);
            printf(" ==> ");
            print_list(i);

        } else {
            printf("|%d, NULL|\n", i);
        }

    }


    printf("+--------------+\n");
}
/* ==================== END: Hash Table Functions ============= */



/* Page to evict is chosen using the optimal (aka MIN) algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	int max_trace = -1;
	int frame = -1; // TODO: Might change this to frame = 0
	for (int i = 0; i < memsize; i++) {
		char *mem_ptr = &physmem[i * SIMPAGESIZE];
		addr_t *vaddr_ptr = (addr_t *)(mem_ptr + sizeof(int));
		addr_t vaddr = *vaddr_ptr;

		int index = search(vaddr >> PAGE_SHIFT);
		if (hash_table[index]->head == NULL) {
			frame = i;
            // printf("%lx this: NULL\n", vaddr);
			// printf("%d====\n", frame);
			return frame;
		} else {
			// printf("%lx this:%d\n", vaddr, hash_table[index]->head->trace);
			if (max_trace < hash_table[index]->head->trace) {
				max_trace = hash_table[index]->head->trace;
				frame = i;
			}
		}
	}
	// printf("%d====\n", frame);
	return frame;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
	int frame_num = p->frame >> PAGE_SHIFT;
	char *mem_ptr = &physmem[frame_num * SIMPAGESIZE];
	addr_t *vaddr_ptr = (addr_t *)(mem_ptr + sizeof(int));
	addr_t vaddr = *vaddr_ptr;

	int index = search(vaddr >> PAGE_SHIFT);

	// If we did find the page (most likely won't need this!)
	if (index != -1) {
		delete(index);
	}
	current_trace ++;
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
	current_trace = 0;
	// TODO:CAN WRITE A FUNCTION THAT GETS THE number of unique pages!
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
		if(buf[0] != '=') {
			sscanf(buf, "%c %lx", &type, &vaddr);
			insert(vaddr >> PAGE_SHIFT);
			current_trace ++;

		} else {
			continue;
		}
	}

	current_trace = 0;
}
