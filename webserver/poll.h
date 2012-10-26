
#ifndef _POLL_H_
#define _POLL_H_

#include "dispatchqueue.h"

typedef struct _Poll* Poll;

typedef enum {
	//
	// This means that a registered block
	// is not removed after it has been triggerd
	//
	kPollRepeatFlag = (1 << 1)
} PollFlags;

//
// Creates a new Poll object
//
// Is a Retainable
//
Poll PollCreate();

//
// Registers a new listener block
// with the specified flags
//
// The block will be dispatched on the given queue.
// Null means that it is called inplace.
//
void PollRegister(Poll poll, int fd, int events, PollFlags flags, DispatchQueue queue, void (^block)(int revents));

//
// Removes a registered listener (if any)
//
void PollUnregister(Poll poll, int fd);

#endif /* _POLL_H_ */
