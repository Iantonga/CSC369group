#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"

#define HSIZE PTRS_PER_PGDIR * PTRS_PER_PGTBL

extern int memsize;

extern int debug;

extern struct frame *coremap;

// extern char *physmem;
// extern char *tracefile;

unsigned int current_trace;



/* Linked List for...*/
struct Node {
	unsigned int trace_index;
	struct node* next;
};


struct hash_tbl_entry {
	addr_t vaddr;
	struct Node *head;
	struct Node *back;
}

/* TODO*/
struct hash_tbl_entry *hash_table[HSIZE];

/*TODO*/
int hash(addr_t vaddr) {
	// TODO
	int multiplier;
	if (PTRS_PER_PGDIR == 4096) {
		multiplier = 16000057;
	} else {
		multiplier = 1000003;
	}
	return (vaddr * multiplier) % (HSIZE);
}

int insert(addr_t vaddr) {

	struct Node *new_node = (struct *Node)malloc(1 * sizeof(struct Node));
	l->trace_index = current_trace;
	l->next = NULL;

	struct hash_tbl_entry *new_ent = (struct *hash_tbl_entry)malloc(1 * sizeof(struct hash_tbl_entry));
	new_ent->vaddr = vaddr;

	if (new_ent->head == NULL) {
		new_ent->head = l;
		new_ent->back = new_node;
	}
	if (hash_table[hash[vaddr]] == NULL) {
		hash_table[hash[vaddr]] = ent;
	} else {

	}
}



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
			return i;
		} else {
			if (max_trace < hash_table[index]->head->trace) {
				max_trace = hash_table[index]->head->trace;
				frame = i;
			}
		}
	}
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
	// Initialize the hash table

	// CAN WRITE A FUNCTION THAT GETS THE number of unique pages!
	memset(hash_table, NULL, HSIZE * sizeof(struct hash_tbl_entry));

	// Open the file
	FILE *tfp;
	if (tracefile != NULL) {
		if((tfp = fopen(tracefile, "r")) == NULL) {
			perror("Error opening tracefile:");
			exit(1);
		}
	}


	char buf[MAXLINE];
	addr_t vaddr = 0;
	char type;
	while(fgets(buf, MAXLINE, tfp) != NULL) {
		if(buf[0] != '=') {
			sscanf(buf, "%c %lx", &type, &vaddr);
			if (search(vaddr) == NULL) {
				insert(vaddr);
			} else {
				update(vaddr);
			}

		} else {
			continue;
		}

	}
}
