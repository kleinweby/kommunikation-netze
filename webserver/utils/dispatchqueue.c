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

#include "dispatchqueue.h"
#include "queue.h"
#include "helper.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <Block.h>

DEFINE_CLASS(DispatchQueue,	
	Queue queue;
	
	pthread_t* threads;
	uint32_t numOfThreads;
	uint32_t maxThreads;
	
	sem_t* semaphore;
);

static void* _DispatchQueueThread(void* ptr);
static void _DispatchQueueDrainAndRelease(DispatchQueue queue);
static void _DisptachQueueDealloc(void* ptr);

DispatchQueue DispatchQueueCreate(DispatchQueueFlags flags)
{
	DispatchQueue queue = malloc(sizeof(struct _DispatchQueue));
	
	memset(queue, 0, sizeof(struct _DispatchQueue));
	
	ObjectInit(queue, _DisptachQueueDealloc);
		
	queue->semaphore = sem_open_anon();
	
	if (queue->semaphore == SEM_FAILED) {
		perror("sem_init");
		Release(queue);
		return NULL;
	}
	
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
	sem_post(queue->semaphore);
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
	sem_wait(queue->semaphore);
	void (^block)() = QueueDrain(queue->queue);
	
	if (block) {
		block();
		Block_release(block);
	}
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
