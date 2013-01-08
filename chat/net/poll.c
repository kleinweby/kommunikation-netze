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

#include "poll.h"
#include "utils/queue.h"
#include "utils/helper.h"

#include <string.h>
#include <pthread.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <Block.h>
#include <assert.h>

struct _PollInfo {
	PollDescriptor descriptorObject; // Unretained
	DispatchQueue queue;
	void (^block)(short revents);
};

DEFINE_CLASS(Poll,	
	pthread_t thread;
	
	//
	// Updates to the poll
	// are enqueue here until they are applied
	//
	Queue updateQueue;
	
	//
	// Fds used to wake up the poll to flush changes
	//
	int updateFDs[2];
	
	//
	// Popolated poll descriptors and
	// related information
	//
	struct pollfd* polls;
	struct _PollInfo* pollInfos;
	uint32_t numOfPolls;
	uint32_t numOfSlots;
);

DEFINE_CLASS(PollDescriptor,
	Poll poll;

	// To later find the pollfd we simply store
	// the pollfd in pollInfos
);

static void* PollThread(void* ptr);
static void PollDealloc(void* ptr);
static void PollApplyUpdates(Poll poll);
static void PollEnqueueUpdate(Poll poll, void (^block)());
static uint32_t PollIndexForDescriptor(PollDescriptor pd);
// These will called by apply updates and therfore are thread safe
static void PollDoRegisterDescriptor(PollDescriptor pd, int fd, short events, DispatchQueue queue, void (^block)(short revents));
static void PollDoAddEvents(PollDescriptor pd, short events);
static void PollDoRemoveEvents(PollDescriptor pd, short events);
static void PollDoRemoveDescriptor(PollDescriptor pd);

static PollDescriptor PollDescriptorCreate(Poll poll);
static void PollDescriptorDealloc(void* ptr);

Poll PollCreate()
{
	Poll poll = malloc(sizeof(struct _Poll));
	
	if (poll == NULL) {
		perror("malloc");
		return NULL;
	}
	
	memset(poll, 0, sizeof(struct _Poll));
	
	ObjectInit(poll, PollDealloc);
	
	poll->numOfSlots = 10;
	poll->numOfPolls = 0;
	poll->polls = malloc(sizeof(struct pollfd) * poll->numOfSlots);
	
	if (poll->polls == NULL) {
		perror("malloc");
		Release(poll);
		return NULL;
	}
	
	memset(poll->polls, 0, sizeof(struct pollfd) * poll->numOfSlots);
	
	poll->pollInfos = malloc(sizeof(struct _PollInfo) * poll->numOfSlots);
	
	if (poll->pollInfos == NULL) {
		perror("malloc");
		Release(poll);
		return NULL;
	}
	
	memset(poll->pollInfos, 0, sizeof(struct _PollInfo) * poll->numOfSlots);
	
	poll->updateQueue = QueueCreate();
	if (poll->updateQueue == NULL) {
		printf("Could not create poll update queue...\n");
		Release(poll);
		return NULL;
	}
	
	if (pipe(poll->updateFDs) < 0) {
		perror("pipe");
		Release(poll);
		return NULL;
	}
	
	setBlocking(poll->updateFDs[0], false);
	setBlocking(poll->updateFDs[1], false);
	
	if (pthread_create(&poll->thread, NULL, PollThread, poll)) {
		perror("pthread_create");
		Release(poll);
		return NULL;
	}
	
	return poll;
}

PollDescriptor PollRegister(Poll poll, int fd, short events, DispatchQueue queue, void (^block)(short revents))
{
	PollDescriptor pd = PollDescriptorCreate(poll);
	
	PollEnqueueUpdate(poll, ^{
		PollDoRegisterDescriptor(pd, fd, events, queue, block);
	});
	
	return pd;
}

void PollDescriptorAddEvent(PollDescriptor pd, short event)
{
	PollEnqueueUpdate(pd->poll, ^{
		PollDoAddEvents(pd, event);
	});
}

void PollDescriptorRemoveEvent(PollDescriptor pd, short event)
{
	PollEnqueueUpdate(pd->poll, ^{
		PollDoRemoveEvents(pd, event);
	});
}

void PollDescritptorRemove(PollDescriptor pd)
{
	PollEnqueueUpdate(pd->poll, ^{
		PollDoRemoveDescriptor(pd);
	});
}

static void* PollThread(void* ptr)
{
	Poll p = ptr;
	int socksToHandle;
	
	// Listen on the update notifing pipe
	// this way, we can quickly react to changed poll desriptors
	// when we would normally hang in the poll timeout
	PollRegister(p, p->updateFDs[0], POLLIN, NULL, ^(short revents){
#pragma unused(revents)
	});
	
	for (;;) {
		// Update the poll before to ensure we get all
		PollApplyUpdates(p);
		
		socksToHandle = poll(p->polls, p->numOfPolls, 1000);
		
		// Update after to not notify deleted poll requests
		PollApplyUpdates(p);
		
		if (socksToHandle < 0) {
			perror("poll");
			return NULL;
		}
		else {
			for (uint32_t i = 0; i < p->numOfPolls && socksToHandle > 0; i++) {
				if (p->polls[i].revents > 0) {
					socksToHandle--;

					// If we have a queue use this
					if (p->pollInfos[i].queue) {
						short revents = p->polls[i].revents;
						void (^block)(short revents) = p->pollInfos[i].block; // Get a reference to properly retain the block
						Dispatch(p->pollInfos[i].queue, ^{
							block(revents);
						});
					}
					else
						p->pollInfos[i].block(p->polls[i].revents);
				}
			}
		}
	}
	
	return NULL;
}

static void PollEnqueueUpdate(Poll poll, void (^block)())
{
	QueueEnqueue(poll->updateQueue, Block_copy(block));
	
	// Notify to reload the poll descritors
	// Not interested in write errors here
	int i = 1;
	write(poll->updateFDs[1], &i, 1);
}

static void PollApplyUpdates(Poll poll)
{
	void (^block)();
	
	// Just read away all the notify-bytes that piled up
	{
		char buffer[255];
		read(poll->updateFDs[0], buffer, 255);
	}
	
	while ((block = QueueDrain(poll->updateQueue)) != NULL) {
		block();
		Block_release(block);
	}
}

static uint32_t PollIndexForDescriptor(PollDescriptor pd)
{
	for(uint32_t i = 0; i < pd->poll->numOfPolls; i++) {
		if (pd->poll->pollInfos[i].descriptorObject == pd) {
			return i;
		}
	}
	
	return UINT32_MAX;
}

static void PollDoRegisterDescriptor(PollDescriptor pd, int fd, short events, DispatchQueue queue, void (^block)(short revents))
{
	// Don't check if pd with fd exists, maybe we want two registratoins
	uint32_t index = pd->poll->numOfPolls;
	pd->poll->numOfPolls++;
	
	// Expand
	if (index >= pd->poll->numOfSlots) {
		uint32_t oldNumOfSlots = pd->poll->numOfSlots;
		pd->poll->numOfSlots *= 2;
		assert(pd->poll->numOfSlots > 0);
		pd->poll->polls = realloc(pd->poll->polls, sizeof(struct pollfd) * pd->poll->numOfSlots);
		assert(pd->poll->polls);
		memset(&pd->poll->polls[oldNumOfSlots], 0, sizeof(struct pollfd) * (pd->poll->numOfSlots - oldNumOfSlots));
		pd->poll->pollInfos = realloc(pd->poll->pollInfos, sizeof(struct _PollInfo) * pd->poll->numOfSlots);
		assert(pd->poll->pollInfos);
		memset(&pd->poll->pollInfos[oldNumOfSlots], 0, sizeof(struct _PollInfo) * (pd->poll->numOfSlots - oldNumOfSlots));
	}
			
	// Now add
	pd->poll->polls[index].fd = fd;
	pd->poll->polls[index].events = events;
	pd->poll->polls[index].revents = 0;
	pd->poll->pollInfos[index].block = Block_copy(block);
	pd->poll->pollInfos[index].descriptorObject = pd; // For finding later
	pd->poll->pollInfos[index].queue = Retain(queue);
}

static void PollDoAddEvents(PollDescriptor pd, short events)
{
	uint32_t index = PollIndexForDescriptor(pd);
	
	pd->poll->polls[index].events |= events;
}

static void PollDoRemoveEvents(PollDescriptor pd, short events)
{
	uint32_t index = PollIndexForDescriptor(pd);
	
	pd->poll->polls[index].events &= ~events;
}

static void PollDoRemoveDescriptor(PollDescriptor pd)
{
	uint32_t i = PollIndexForDescriptor(pd);

	if (pd->poll->pollInfos[i].block)
		Block_release(pd->poll->pollInfos[i].block);
			
	// If this is not the last
	// you bring the last one here, to avoid large copies
	if (i != pd->poll->numOfPolls - 1) {
		memcpy(&pd->poll->polls[i], &pd->poll->polls[pd->poll->numOfPolls - 1], sizeof(struct pollfd));
		memcpy(&pd->poll->pollInfos[i], &pd->poll->pollInfos[pd->poll->numOfPolls - 1], sizeof(struct _PollInfo));
		memset(&pd->poll->polls[pd->poll->numOfPolls - 1], 0, sizeof(struct pollfd));
		memset(&pd->poll->pollInfos[pd->poll->numOfPolls - 1], 0, sizeof(struct _PollInfo));
	}
	// Clear out the data
	else {
		memset(&pd->poll->polls[i], 0, sizeof(struct pollfd));
		memset(&pd->poll->pollInfos[i], 0, sizeof(struct _PollInfo));
	}
						
	pd->poll->numOfPolls--;
}

static void PollDealloc(void* ptr)
{
	Poll poll = ptr;

	Release(poll->updateQueue);
	close(poll->updateFDs[0]);
	close(poll->updateFDs[1]);
	
	if (poll->polls)
		free(poll->polls);
	
	if (poll->pollInfos)
		free(poll->pollInfos);
	
	free(poll);
}

static PollDescriptor PollDescriptorCreate(Poll poll)
{
	PollDescriptor pd = malloc(sizeof(struct _PollDescriptor));
	
	if (pd == NULL) {
		perror("malloc");
		return NULL;
	}
	
	memset(pd, 0, sizeof(struct _PollDescriptor));
	
	ObjectInit(pd, PollDescriptorDealloc);
	pd->poll = poll;
	
	return pd;
}

static void PollDescriptorDealloc(void* ptr)
{
	PollDescriptor pd = ptr;
	
	free(pd);
}
