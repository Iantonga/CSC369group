#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "pagetable.h"

#define FALSE 0
#define TRUE 1


extern int memsize;

extern int debug;

extern struct frame *coremap;

/* A wrapper of the data structure circular queue. */
typedef struct {
	// The index of the first element of the queue
    int begin;

	// The index of the next available element of the queue
    int end;

	// The capacity of the queue
    int size;

	// Whether the queue is full or not
    int isfull;

	// The content of the queue
    int *content;
} Queue;

/* Initialize circular queue q with capacity qsize */
void init(Queue* q, int qsize) {
    q->begin = 0;
    q->end = 0;
    q->size = qsize;
    q->isfull = FALSE;
    q->content = (int*)malloc(qsize * sizeof(int));
    memset(q->content, -1, qsize * sizeof(int));
}

/* Enqueue element elem in circular queue q*/
int enqueue(Queue* q, int elem) {
    int t = q->content[q->end];
    q->content[q->end] = elem;

    if (q->isfull) {
        q->begin = (q->begin + 1) % q->size;
    }

    if (q->end == q->size - 1) {
        q->isfull = TRUE;
    }

    q->end = (q->end + 1) % q->size;
    return t;
}

/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {

	return 0;
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
}
