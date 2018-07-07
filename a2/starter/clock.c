#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int clock_hand;
/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
	int evicted_frame = clock_hand;
	while (coremap[clock_hand].pte->frame & PG_REF) {
		coremap[clock_hand].pte->frame = coremap[clock_hand].pte->frame ^ PG_REF;
		clock_hand = (clock_hand + 1) % memsize;
	}
	printf("rvict %d\n", clock_hand);
	return clock_hand;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
	// p is in the memory then we have a hit
	if (p->frame & PG_VALID) {
		if (!(p->frame & PG_REF)) {
			p->frame = p->frame | PG_REF;
		}
	} else {
		printf("===== %d\n", coremap[clock_hand].pte->frame & PG_REF);
		if (coremap[clock_hand].pte->frame & PG_REF) {
				printf("we are here\n" );
				while (coremap[clock_hand].pte->frame & PG_REF) {
					coremap[clock_hand].pte->frame = coremap[clock_hand].pte->frame ^ PG_REF;
					clock_hand = (clock_hand + 1) % memsize;
				}
		} else {
			coremap[clock_hand].pte->frame = coremap[clock_hand].pte->frame | PG_REF;
			clock_hand = (clock_hand + 1) % memsize;
		}
	}
	for (int i = 0; i < memsize; i++) {
		if(coremap[i].pte != 0) {
			printf("%d\n", coremap[i].pte->frame & PG_REF);
		} else {
			printf("empty\n");
		}

	}
	printf("clock hand: %d\n", clock_hand);
	printf("-------\n\n");
	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm.
 */
void clock_init() {
	clock_hand = 0;
}
