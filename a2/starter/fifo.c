#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "pagetable.h"

extern int memsize;

extern int debug;

extern struct frame *coremap;


/* We assume that the pages are allocated to the very first
available frame since in 'allocate_frame' in order to find a
free frame we check from 0 to memsize - 1.*/
int queue_front;


/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {
    int frame_to_be_evicted = queue_front;
    // Move the queue_front to the next position
    // after eviction!
    queue_front = (queue_front + 1) % memsize;
    return frame_to_be_evicted;
}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {
	return;
}

/* Initialize any data structures needed for this
 * replacement algorithm
 */
void fifo_init() {
    // Start from the beginning of the queue.
    queue_front = 0;
}
