// Copyright (c) 2012, Christian Speich <christian@spei.ch>
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "queue.h"
#include "helper.h"

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

//
// dequeue and queue borrowed from http://www.cs.rochester.edu/research/synchronization/pseudocode/queues.html
//

struct _QueueElement {
	void* element;
	
	struct _QueueElement* next;
	bool dequeued;
};

DEFINE_CLASS(Queue,
	struct _QueueElement* head;
	struct _QueueElement* tail;
);

Queue QueueCreate() {
	Queue queue = malloc(sizeof(struct _Queue));
	
	memset(queue, 0, sizeof(struct _Queue));
	
	ObjectInit(queue, (void (*)(void *))QueueDestroy);
	
	struct _QueueElement* node = malloc(sizeof(struct _QueueElement));
	memset(node, 0, sizeof(struct _QueueElement));
	
	queue->head = node;
	queue->tail = node;
	
	return queue;
}

void QueueDestroy(Queue queue) {	
	struct _QueueElement* element = queue->head;
	while (element != NULL) {
		struct _QueueElement* old = element;
		element = old->next;
		free(old);
	}
	
	free(queue);
}

void QueueEnqueue(Queue queue, void* e) {
	struct _QueueElement* node = malloc(sizeof(struct _QueueElement));
	struct _QueueElement* tail;
	struct _QueueElement* next;
	
	memset(node, 0, sizeof(struct _QueueElement));
	
	node->element = e;
	node->dequeued = false;
	
	while(1) { // Keep trying until Enqueue is done
		tail = queue->tail; // Read Tail.ptr and Tail.count together
		next = tail->next; // Read next ptr and count fields together
		if (tail == queue->tail) {// Are tail and next consistent?
			// Was Tail pointing to the last node?
			if (next == NULL) {
				// Try to link node at the end of the linked list
				if (__sync_bool_compare_and_swap(&tail->next, next, node)) {
					break; // Enqueue is done.  Exit loop
				}
			}
			else { // Tail was not pointing to the last node
				// Try to swing Tail to the next node
				__sync_bool_compare_and_swap(&queue->tail, tail, next);
			}
		}
	}
	
	// Enqueue is done.  Try to swing Tail to the inserted node
	__sync_bool_compare_and_swap(&queue->tail, tail, node);
}

void* QueueDrain(Queue queue)
{
	void* element = NULL;
	struct _QueueElement* tail;
	struct _QueueElement* head;
	struct _QueueElement* next;
	
	while (1) { // Keep trying until Dequeue is done
		head = queue->head; // Read Head
		tail = queue->tail; // Read Tail
		next = head->next;
		if (head == queue->head) { // Are head, tail, and next consistent?
			if (head == tail) { // Is queue empty or Tail falling behind?
				if (next == NULL) // Is queue empty?
					return NULL; // Queue is empty, couldn't dequeue
				
				// Tail is falling behind.  Try to advance it
				__sync_bool_compare_and_swap(&queue->tail, tail, next);
			}
			else {
				// Read value before CAS
				// Otherwise, another dequeue might free the next node
				element = next->element;
				// Try to swing Head to the next node
				if (__sync_bool_compare_and_swap(&queue->head, head, next))
					break; // Dequeue is done.  Exit loop
			}
		}
	}
	assert(next->dequeued == false);
	next->dequeued = true;
	free(head);
	return element;
}
