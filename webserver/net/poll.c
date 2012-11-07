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
	PollFlags flags;
	DispatchQueue queue;
	void (^block)(short revents);
};

struct _PollUpdate {
	struct pollfd poll;
	struct _PollInfo pollInfo;
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

static void* PollThread(void* ptr);
static void PollDealloc(void* ptr);
static void PollApplyUpdates(Poll poll);

Poll PollCreate()
{
	Poll poll = malloc(sizeof(struct _Poll));
	
	memset(poll, 0, sizeof(struct _Poll));
	
	ObjectInit(poll, PollDealloc);
	
	poll->numOfSlots = 10;
	poll->numOfPolls = 0;
	poll->polls = malloc(sizeof(struct pollfd) * poll->numOfSlots);
	poll->pollInfos = malloc(sizeof(struct _PollInfo) * poll->numOfSlots);
	poll->updateQueue = QueueCreate();
	
	pipe(poll->updateFDs);
	
	setBlocking(poll->updateFDs[0], false);
	setBlocking(poll->updateFDs[1], false);
	
	if (pthread_create(&poll->thread, NULL, PollThread, poll)) {
		perror("pthread_create");
		Release(poll);
		return NULL;
	}
	
	return poll;
}

void PollRegister(Poll poll, int fd, short events, PollFlags flags, DispatchQueue queue, void (^block)(short revents))
{
	struct _PollUpdate* update = malloc(sizeof(struct _PollUpdate));
	
	memset(update, 0, sizeof(struct _PollUpdate));
	
	update->poll.fd = fd;
	update->poll.events = events;
	update->pollInfo.block = Block_copy(block);
	if (queue)
		update->pollInfo.queue = Retain(queue);
	update->pollInfo.flags = flags;
	
	QueueEnqueue(poll->updateQueue, update);
	
	// Notify to reload the poll descritors
	int i = 1;
	write(poll->updateFDs[1], &i, 1);
}

void PollUnregister(Poll poll, int fd)
{
	struct _PollUpdate* update = malloc(sizeof(struct _PollUpdate));
	
	memset(update, 0, sizeof(struct _PollUpdate));
	
	update->poll.fd = fd;
	
	QueueEnqueue(poll->updateQueue, update);
	
	// Notify to reload the poll descritors
	int i = 1;
	write(poll->updateFDs[1], &i, 1);
}

static void* PollThread(void* ptr)
{
	Poll p = ptr;
	int socksToHandle;
	
	PollRegister(p, p->updateFDs[0], POLLIN, kPollRepeatFlag, NULL, ^(short revents){
#pragma unused(revents)
	});
	
	for (;;) {
		PollApplyUpdates(p);
		
		socksToHandle = poll(p->polls, p->numOfPolls, 1000);
		
		PollApplyUpdates(p);
		
		if (socksToHandle < 0) {
			perror("poll");
			return NULL;
		}
		else {
			for (uint32_t i = 0; i < p->numOfPolls && socksToHandle > 0; i++) {
				if (p->polls[i].revents > 0) {
					socksToHandle--;
					
					// Dont repeat so remove it
					// We need to enqueue the update before to let the block
					// reregister itself
					if ((p->pollInfos[i].flags & kPollRepeatFlag) == 0) {
						assert(p->updateFDs[0] != p->polls[i].fd);
						PollUnregister(p, p->polls[i].fd);
					}
					
					// If we have a queue use this
					if (p->pollInfos[i].queue) {
						short revents = p->polls[i].revents;
						void (^block)(short revents) = p->pollInfos[i].block;
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

static void PollApplyUpdates(Poll poll)
{
	struct _PollUpdate *update;
	
	{
		char buffer[255];
		read(poll->updateFDs[0], buffer, 255);
	}
	
	while ((update = QueueDequeue(poll->updateQueue)) != NULL) {
		// Add/Update
		if (update->pollInfo.block) {
			uint32_t foundIndex = UINT32_MAX;
			for(uint32_t i = 0; i < poll->numOfPolls; i++) {
				if (poll->polls[i].fd == update->poll.fd) {
					foundIndex = i;
					break;
				}
			}
	
			// Not found, add to the end
			if (foundIndex == UINT32_MAX) {
				foundIndex = poll->numOfPolls;
				poll->numOfPolls++;
			}
	
			// Expand
			if (foundIndex >= poll->numOfSlots) {
				poll->numOfSlots *= 2;
				poll->polls = realloc(poll->polls, sizeof(struct pollfd) * poll->numOfSlots);
				poll->pollInfos = realloc(poll->pollInfos, sizeof(struct _PollInfo) * poll->numOfSlots);
			}
			
			// Now update/add
			memcpy(&poll->polls[foundIndex].fd, &update->poll, sizeof(struct pollfd));
			if (poll->pollInfos[foundIndex].block) {
				Block_release(poll->pollInfos[foundIndex].block);
			}
			// Copy occoured when enqueued to update
			poll->pollInfos[foundIndex].block = update->pollInfo.block;
			poll->pollInfos[foundIndex].flags = update->pollInfo.flags;
			if (poll->pollInfos[foundIndex].queue) {
				Release(poll->pollInfos[foundIndex].queue);
			}
			poll->pollInfos[foundIndex].queue = update->pollInfo.queue;
		}
		// Remove
		else {
			for(uint32_t i = 0; i < poll->numOfPolls; i++) {
				if (poll->polls[i].fd == update->poll.fd) {
					if (poll->pollInfos[i].block)
						Block_release(poll->pollInfos[i].block);
			
					// If this is not the last
					// you bring the last one here, to avoid large copies
					if (i != poll->numOfPolls - 1) {
						memcpy(&poll->polls[i], &poll->polls[poll->numOfPolls - 1], sizeof(struct pollfd));
						memcpy(&poll->pollInfos[i], &poll->pollInfos[poll->numOfPolls - 1], sizeof(struct _PollInfo));
						memset(&poll->polls[poll->numOfPolls - 1], 0, sizeof(struct pollfd));
						memset(&poll->pollInfos[poll->numOfPolls - 1], 0, sizeof(struct _PollInfo));
					}
					// Clear out the data
					else {
						memset(&poll->polls[i], 0, sizeof(struct pollfd));
						memset(&poll->pollInfos[i], 0, sizeof(struct _PollInfo));
					}
						
					poll->numOfPolls--;
					break;
				}
			}
		}
		
		free(update);
	}
}

static void PollDealloc(void* ptr)
{
#pragma unused(ptr)
}
