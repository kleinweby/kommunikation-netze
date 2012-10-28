#include "dispatchqueue.h"
#include "retainable.h"
#include "queue.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <Block.h>

struct _DispatchQueue {
	Retainable retainable;
	
	Queue queue;
	
	pthread_t* threads;
	uint32_t numOfThreads;
	uint32_t maxThreads;
};

static void* _DispatchQueueThread(void* ptr);
static void _DispatchQueueDrainAndRelease(DispatchQueue queue);
static void _DisptachQueueDealloc(void* ptr);

DispatchQueue DispatchQueueCreate(DispatchQueueFlags flags)
{
	DispatchQueue queue = malloc(sizeof(struct _DispatchQueue));
	
	memset(queue, 0, sizeof(struct _DispatchQueue));
	
	RetainableInitialize(&queue->retainable, _DisptachQueueDealloc);
	queue->queue = QueueCreate();
	
	queue->maxThreads = (flags & kDispatchQueueSerial) ? 1 : 10;
	
	queue->threads = malloc(sizeof(pthread_t) * queue->maxThreads);
	
	// Start our threads
	for (uint32_t i = 0; i < queue->maxThreads; i++) {
		if (pthread_create(&queue->threads[i], NULL, _DispatchQueueThread, queue)) {
			perror("pthread_create");
			Release(queue);
			return NULL;
		}
		queue->numOfThreads++;
	}
	
	return queue;
}

void Dispatch(DispatchQueue queue, void(^block)())
{
	QueueEnqueue(queue->queue, Block_copy(block));
}

static void* _DispatchQueueThread(void* ptr)
{
	DispatchQueue queue = ptr;
	
	for (;;) {
		_DispatchQueueDrainAndRelease(queue);
	}
}

static void _DispatchQueueDrainAndRelease(DispatchQueue queue)
{
	void (^block)() = QueueDrain(queue->queue);
	
	block();
	Block_release(block);
}

static void _DisptachQueueDealloc(void* ptr)
{
	DispatchQueue queue = ptr;
	// We need to stop all remaining threads, so we just enqueue
	// One exit block for everh thread
	for (uint32_t i = 0; i < queue->numOfThreads; i++) {
		Dispatch(queue, ^ __attribute__((noreturn)) {
			pthread_exit(NULL);
		});
	}
	
	// Now we join all the threads to let them finish before
	// we remove any data
	for (uint32_t i = 0; i < queue->numOfThreads; i++) {
		pthread_join(queue->threads[i], NULL);
	}
	
	// Now we can release the data
	Release(queue->queue);
	free(queue->threads);
	free(queue);
}
