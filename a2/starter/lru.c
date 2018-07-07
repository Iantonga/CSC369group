#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

/* We use doubly linkedlist "stack" where the bottom points to the bottom of the "stack"
* points to the top of the "stack"!
*/
struct frame *bottom;
struct frame *top;

/* This indicates if this is the first access to memory, and tells us to initialize the top and bottom pointer*/
unsigned int size = 0;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	if (size == 1) {
		return 0;
	}
	bottom->next->prev = NULL;
	struct frame *temp = bottom->next;
	bottom->next = NULL;
	int frame_num = bottom->pte->frame >> PAGE_SHIFT;
	bottom = temp;
	return frame_num;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	int frame_num = p->frame >> PAGE_SHIFT;
	// first time brining a node in
	if (size == 0) {
		bottom = &coremap[frame_num];
		top = &coremap[frame_num];
		coremap[frame_num].prev = NULL;
		coremap[frame_num].next = NULL;
		size ++;
	// cold miss and capacity miss
	} else if (!(p->frame & PG_VALID)) {
		if (size < memsize) {
			size ++;
		}
		top->next = &coremap[frame_num];
		coremap[frame_num].prev = top;
		coremap[frame_num].next = NULL;
		top = &coremap[frame_num];
	// hit

	} else if (top == &coremap[frame_num]) {
		// Do nothing
	} else if (bottom == &coremap[frame_num]) {
		coremap[frame_num].next->prev = NULL;
		bottom = coremap[frame_num].next;
		coremap[frame_num].next = NULL;
		top->next = &coremap[frame_num];
		coremap[frame_num].prev = top;
		top = &coremap[frame_num];

	} else {
		coremap[frame_num].prev->next = coremap[frame_num].next;
		coremap[frame_num].next->prev = coremap[frame_num].prev;
		coremap[frame_num].prev = top;
		top->next = &coremap[frame_num];
		coremap[frame_num].next = NULL;
		top = &coremap[frame_num];
	}
	return;
}


/* Initialize any data structures needed for this
 * replacement algorithm
 */
void lru_init() {
}
