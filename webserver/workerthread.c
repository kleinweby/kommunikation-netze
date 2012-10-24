#include "workerthread.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct _WorkPackage* WorkPackage;

struct _WorkPackage {
	Worker worker;
	void* userInfo;
	
	WorkPackage next;
};

struct _WorkerThreads {
	pthread_t* threads;
	int startNumThreads;
	int maxNumThreads;
	
	WorkPackage queue;
	WorkPackage queueEnd;
	pthread_mutex_t queueMutex;
	pthread_cond_t queueCondition;
};

typedef struct _WorkerThreads* WorkerThreads;

static WorkerThreads workerThreads;

static void* WorkerSchedule();

void WorkerThreadsInitialize(int maxNumThreads)
{
	workerThreads = malloc(sizeof(struct _WorkerThreads));
	
	workerThreads->startNumThreads = maxNumThreads;
	workerThreads->maxNumThreads = maxNumThreads;
	workerThreads->threads = malloc(sizeof(pthread_t) * workerThreads->maxNumThreads);
	
	pthread_mutex_init(&workerThreads->queueMutex, 0);
	pthread_cond_init(&workerThreads->queueCondition, 0);
	for (int i = 0; i < workerThreads->startNumThreads; i++) {
		if (pthread_create(&workerThreads->threads[i], NULL, WorkerSchedule, NULL)) {
			perror("pthread_create");
		}
	}
}

void WorkerThreadsEnqueue(Worker worker, void* userInfo)
{
	WorkPackage package = malloc(sizeof(struct _WorkPackage));
	
	memset(package, 0, sizeof(struct _WorkPackage));
	package->worker = worker;
	package->userInfo = userInfo;
	
	pthread_mutex_lock(&workerThreads->queueMutex);
	if (workerThreads->queue) {
		workerThreads->queueEnd->next = package;
		workerThreads->queueEnd = package;
	}
	else {
		workerThreads->queue = package;
		workerThreads->queueEnd = package;
	}
	pthread_cond_signal(&workerThreads->queueCondition);
	pthread_mutex_unlock(&workerThreads->queueMutex);
}

static void* WorkerSchedule()
{
	for (;;) {
		WorkPackage package;
		
		pthread_mutex_lock(&workerThreads->queueMutex);
		
		while (!workerThreads->queue) {
			pthread_cond_wait(&workerThreads->queueCondition, &workerThreads->queueMutex);
		}
		
		package = workerThreads->queue;
		workerThreads->queue = package->next;
		if (package == workerThreads->queueEnd)
			workerThreads->queueEnd = NULL;
		pthread_mutex_unlock(&workerThreads->queueMutex);
		
		package->worker(package->userInfo);
		free(package);
	}
}
