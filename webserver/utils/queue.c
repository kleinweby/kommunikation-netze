#include "queue.h"
#include "retainable.h"

#include <string.h>
#include <stdlib.h>
#include <pthread.h>

struct _QueueElement {
	void* element;
	
	struct _QueueElement* next;
};

struct _Queue {
	Retainable retainable;
	
	pthread_mutex_t mutex;
	pthread_cond_t condition;
	
	struct _QueueElement* head;
	struct _QueueElement* tail;
};

Queue QueueCreate() {
	Queue queue = malloc(sizeof(struct _Queue));
	
	memset(queue, 0, sizeof(struct _Queue));
	
	RetainableInitialize(&queue->retainable, (void (*)(void *))QueueDestroy);
	
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
