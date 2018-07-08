#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

/* The index in the coremap where this clock hand is "poitning" to.*/
int clock_hand;

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int clock_evict() {
	while (coremap[clock_hand].pte->frame & PG_REF) {
		coremap[clock_hand].pte->frame = coremap[clock_hand].pte->frame ^ PG_REF;
		clock_hand = (clock_hand + 1) % memsize;
	}
	return clock_hand;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
	// If this p is in memroy then we have a hit
	if (p->frame & PG_VALID) {
		// If this p was not referenced then we set the bit to 1.
		if (!(p->frame & PG_REF)) {
			p->frame = p->frame | PG_REF;
		}

	// This is the case where p was not in the memroy
	} else {
		// If the frame that the clock_hand is "poiting" to is referenced
		// then we ...
		if (coremap[clock_hand].pte->frame & PG_REF) {
				while (coremap[clock_hand].pte->frame & PG_REF) {
					coremap[clock_hand].pte->frame = coremap[clock_hand].pte->frame ^ PG_REF;
					clock_hand = (clock_hand + 1) % memsize;
				}
		// This is the case where in the eviction function we change R=1 to R=0
		// then here we set ...
		} else {
			coremap[clock_hand].pte->frame = coremap[clock_hand].pte->frame | PG_REF;
			clock_hand = (clock_hand + 1) % memsize;
		}
	}
	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm.
 */
void clock_init() {
	clock_hand = 0;
}
