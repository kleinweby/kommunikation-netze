#include "workerthread.h"

#include "queue.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <Block.h>

struct _WorkerThreads {
	pthread_t* threads;
	int startNumThreads;
	int maxNumThreads;
	
	Queue queue;
};

typedef struct _WorkerThreads* WorkerThreads;

static WorkerThreads workerThreads;

static void* WorkerSchedule();
static void WorkerBlockDrainAndRelease(Queue queue);

void WorkerThreadsInitialize(int maxNumThreads)
{
	workerThreads = malloc(sizeof(struct _WorkerThreads));
	
	workerThreads->startNumThreads = maxNumThreads;
	workerThreads->maxNumThreads = maxNumThreads;
	workerThreads->threads = malloc(sizeof(pthread_t) * workerThreads->maxNumThreads);
	workerThreads->queue = QueueCreate();
	
	for (int i = 0; i < workerThreads->startNumThreads; i++) {
		if (pthread_create(&workerThreads->threads[i], NULL, WorkerSchedule, NULL)) {
			perror("pthread_create");
		}
	}
}

void WorkerThreadsEnqueue(WorkerBlock block)
{
	QueueEnqueue(workerThreads->queue, Block_copy(block));
}

static void* WorkerSchedule()
{
	for (;;) {
		WorkerBlockDrainAndRelease(workerThreads->queue);
	}
}

static void WorkerBlockDrainAndRelease(Queue queue)
{
	WorkerBlock block = QueueDrain(queue);
	
	block();
	Block_release(block);
}
