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

#include <string.h>
#include <stdlib.h>
#include <pthread.h>

struct _QueueElement {
	void* element;
	
	struct _QueueElement* next;
};

DEFINE_CLASS(Queue,	
	pthread_mutex_t mutex;
	pthread_cond_t condition;
	
	struct _QueueElement* head;
	struct _QueueElement* tail;
);

Queue QueueCreate() {
	Queue queue = malloc(sizeof(struct _Queue));
	
	memset(queue, 0, sizeof(struct _Queue));
	
	ObjectInit(queue, (void (*)(void *))QueueDestroy);
	
	pthread_mutex_init(&queue->mutex, 0);
	pthread_cond_init(&queue->condition, 0);
	
	return queue;
}

void QueueDestroy(Queue queue) {
	pthread_mutex_lock(&queue->mutex);
	
	struct _QueueElement* element = queue->head;
	while (element != NULL) {
		struct _QueueElement* old = element;
		free(old);
		element = old->next;
	}
	
	pthread_mutex_unlock(&queue->mutex);
	free(queue);
}

void QueueEnqueue(Queue queue, void* e) {
	struct _QueueElement* element = malloc(sizeof(struct _QueueElement));
	
	memset(element, 0, sizeof(struct _QueueElement));
	
	element->element = e;
	
	pthread_mutex_lock(&queue->mutex);
	if (queue->tail) {
		queue->tail->next = element;
		queue->tail = element;
	}
	else {
		queue->head = element;
		queue->tail = element;
	}
	pthread_mutex_unlock(&queue->mutex);
	pthread_cond_broadcast(&queue->condition);
}

void* QueueDrain(Queue queue) {
	struct _QueueElement* element = NULL;
	void* e;
	
	pthread_mutex_lock(&queue->mutex);
	
	while ((element = queue->head) == NULL) {
		pthread_cond_wait(&queue->condition, &queue->mutex);
	}

	queue->head = element->next;
	if (element == queue->tail)
		queue->tail = NULL;
	pthread_mutex_unlock(&queue->mutex);
	
	e = element->element;
	
	free(element);
	return e;
}

void* QueueDequeue(Queue queue)
{
	struct _QueueElement* element = NULL;
	void* e = NULL;
	
	pthread_mutex_lock(&queue->mutex);
	
	element = queue->head;
	
	if (element) {
		queue->head = element->next;
		if (element == queue->tail)
			queue->tail = NULL;
	}
	
	pthread_mutex_unlock(&queue->mutex);
	
	if (element) {
		e = element->element;
	
		free(element);
	}
	
	return e;
}
