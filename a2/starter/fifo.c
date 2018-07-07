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


/* We assume that the pages are allocated to the very first
available frame since in 'allocate_frame' in order to find a
free frame we check from 0 to memsize - 1.*/
int queue_front;

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

	// Frame number that contains the page to evict
	unsigned int frame_to_be_evicted;

} Queue;

/* Global circular queue */
Queue q;

/* Initialize circular queue q with capacity qsize */
void init_queue(Queue* q, int qsize) {
    q->begin = 0;
    q->end = 0;
    q->size = qsize;
    q->isfull = FALSE;
    q->content = (int*)malloc(qsize * sizeof(int));

	// It's set to -1 because the queue is not full yet!
	q->frame_to_be_evicted = -1;
    memset(q->content, -1, qsize * sizeof(int));
}

/* Enqueue element elem in circular queue q*/
int enqueue(Queue* q, int elem) {
    q->content[q->end] = elem;

    if (q->isfull) {
        q->begin = (q->begin + 1) % q->size;
    }

    if (q->end == q->size - 1) {
        q->isfull = TRUE;
    }

    q->end = (q->end + 1) % q->size;
	int t = q->content[q->end];
    return t;
}

/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {
    int frame_to_be_evicted = queue_front;
    // Move the queue_front to the next position
    queue_front = (queue_front + 1) % memsize;
	// return q.frame_to_be_evicted;
    return frame_to_be_evicted;
}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {

	// unsigned int frame_num = p->frame >> PAGE_SHIFT;
	// q.frame_to_be_evicted = enqueue(&q, frame_num);
	return;
}

/* Initialize any data structures needed for this
 * replacement algorithm
 */
void fifo_init() {
	// init_queue(&q, memsize);
    queue_front = 0;
}
